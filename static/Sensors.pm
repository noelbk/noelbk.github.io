#! /usr/bin/perl -w

=pod

=head1 NAME

    gksensor - gkgun's plug-in sensor modules

=head1 DESCRIPTION

When gkgun starts, it execs all programs in /usr/share/gksensor and
monitors their stdout.  Any time a program outputs a line like this:

 sensor_name: sensor_value

..it is recorded in the sensor database for the current video.

=head1 AVAILABLE SENSOR PACKAGES

=over 4

=item gksensor-gps-lassensq 

    installs a sensor for the Lassen SQ GPS

=item gksensor-gps-nmea 

    generic GPS NMEA sensor for gkgun

=item tsip2nmea 

    converts Trimble's TSIP protocol to NMEA

=head1 AUTHOR

 Noel Burton-Krahn <noel@burton-krahn.com>
 Copyright 2004 Gatekeeper Systems, Inc

=head1 SEE ALSO


=cut

package GKGun;
use strict;
use IO::File;
use POSIX qw(:signal_h :sys_wait_h :errno_h);
use Fcntl;
use SubProc;

# start all sensor modules in /usr/share/gksensor.  Each sensor
# module is a process which emits lines like "sensor: value"
# asynchronously.  The functions sensor_modules_select and
# sensors_modules_selected use select() to find which sensors have
# produced readings, then call $GKGun::sensor() to store the readings
# in the database.
 
sub sensor_modules_init() {
    my($self) = shift;
    $self->{sensor_modules_dir} ||= "/usr/share/gksensor";
    my($dir) = $self->{sensor_modules_dir};
    my($file, $path, $io, $pid, $proc);

    # debug
    #warn("sensor_modules_init: dir=$dir");

    opendir(D, $dir);
    while( $file = readdir(D) ) {
	$path = "$dir/$file";
	next unless( -x $path 
		     && $file =~ /^[-.\w]+$/ 
		     && ! -d $path );

	$proc = SubProc->new($path);

	$io = $proc->pty();
	$pid = $proc->pid();

	if ( $path =~ /sbox$/ ) {
	    $self->{sbox} = $proc;
	}

	my($flags);
	$flags = fcntl($io, F_GETFL, 0)
	    or die "Couldn't get flags for $io : $!\n";
	$flags |= O_NONBLOCK;
	fcntl($io, F_SETFL, $flags)
	    or die "Couldn't set flags for $io: $!\n";

	$self->{sensor_modules_by_fd}{$io->fileno()} = {
	    proc => $proc,
	    io => $io,
	    pid => $pid,
	    path => $path,
	    buf => '',
	};


	# debug
	#warn("sensor_modules_init: fd=" . $io->fileno() . " pid=$pid path=$path");
    }
    closedir(D);
    if ( ! $self->{sbox} ) {
	warn("no sbox? $ENV{SENSOR_BOX}");
    }
}

sub sensor_modules_fini() {
    my($self) = shift;
    my($k, $x, $i, @sigs, $t, $n);

    # debug
    #warn("sensor_modules_fini: killing");

    # let SubProc clean up
    while( ($k, $x) = each(%{$self->{sensor_modules_by_fd}}) ) {
	$x->{proc}->kill(SIGTERM);
    }

    $self->{sensor_modules_by_fd} = undef;

    $self->{sbox} = undef

    # debug
    #warn("sensor_modules_fini: done");
}

sub sensor_modules_select() {
    my($self) = shift;
    my($rin) = @_;
    my($fd, $x);

    while( ($fd, $x) = each(%{$self->{sensor_modules_by_fd}}) ) {
	if( $x->{io} ) {
	    vec($rin, $fd, 1) = 1;

	    # debug
	    #warn("sensor_modules_select: fd=$fd path=$x->{path}");
	}
    }
    return $rin
}

sub sensor_sbox_seconds () {
    my($self) = shift;
    my($s) = @_;

    return if ( ! $self->{sbox} );

    if( @_ == 0 ) {
	#warn("sensor_sbox_seconds: ...  $self->{sbox_seconds}") if ( $self->{sbox_seconds} );
	$self->{sbox}{pty}->print("seconds\n");
	# last value available
	return $self->{sbox_seconds};
    }
    elsif( @_ == 1 ) {
	#warn("sensor_sbox_seconds: writing... $s");
	$self->{sbox}{pty}->print("seconds= $s\n");

	$self->{sbox_seconds} = $s;
	return $self->{sbox_seconds};
    }
    else {
	warn("sensor_sbox_seconds: wrong # args! ($s ...)");
    }
}

sub sensor_sbox_input () {
    my($self) = shift;
    my($s) = @_;

    return if ( ! $self->{sbox} );

    # just tickle sbox to give us an update
    if( @_ == 0 ) {
	warn("sensor_sbox_input: ...");
	$self->{sbox}{pty}->print("input\n");
    }
    else {
	warn("sensor_sbox_input: wrong # args! ($s ...)");
    }
}

sub sensor_sbox_output () {
    my($self) = shift;
    my($output, $slow, $fast) = @_;

    $output ||= 0;
    $slow ||= 0;
    $fast ||= 0;

    return if ( ! $self->{sbox} );

    # For the output byte (1 = on):
    # Bit 0 = GPS/Speed sensor power
    # Bit 1 = Alarm led
    # Bit 2 = Record led
    # The rest aren't used.

    if( @_ == 0 ) {
	# just tickle sbox to give us an update
	#warn("sensor_sbox_output: ...");
	$self->{sbox}{pty}->print("output\n");
    }
    elsif( @_ == 3 ) {
	#warn("sensor_box: writing... o$output s$slow f$fast $self->{sbox}");
	$self->{sbox}{pty}->print("output= $output $slow $fast\n");
    }
    else {
	warn("sensor_sbox_output: wrong # args ($output ... " . (0 + @_));
    }
}

sub sigfig_count {
    my($val) = @_;
    $val =~ s/^([.\d]+).*/$1/;
    $val =~ s/[^\d]+//g;
    return length($val);
}

sub sigfig_int {
    my($digits, $val) = @_;
    
    $val = sprintf("%.${digits}g", $val);
    if( $val =~ /(.*)e([-+]?)0?(\d+)/i ) {
	$val = $1 * (10 ** ("$2$3"));
    }
    return $val;
}

sub units_convert {
    my($self) = shift;
    my($src) = @_;

    $src =~ /^([\d.]+)(knots|kts|mph|kph)$/ 
	or return $src;
    
    my($src_val, $src_units) = ($1, $2);
    my($dst_val, $dst_units, $dst);
    
    my($src_len) = sigfig_count($src_val)+1;

    # convert everything to kph first
    if( $src_units eq 'kph' ) {
    }
    elsif( $src_units eq 'mph' ) {
	$src_val *= 1.609;
	$src_units = 'kph';
    }
    elsif( $src_units eq 'knots' || $src_units eq 'kts' ) {
	$src_val *= 1.852;
	$src_units = 'kph';
    }
    else {
	# no match!  return the unconverted string
	return $src;
    }
    
    # convert from kph to mph/kts/etc
    if( $src_units eq 'kph' ) {
	$dst_units = $self->config('metric');
	$dst_val = $src_val;
	$dst = $src;
	if( $dst_units == &GKUNITS_METRIC ) {
	    # already in kph
	    $dst_units = 'kph';
	}
	elsif( $dst_units == &GKUNITS_US ) {
	    $dst_val /= 1.609;
	    $dst_units = 'mph';
	}
	elsif( $dst_units == &GKUNITS_KNOTS ) {
	    $dst_val /= 1.852;
	    $dst_units = 'kts';
	}

	if( $dst_units ) {
	    # kludge - retain the same number of significant digits
	    $dst_val = sigfig_int($src_len, $dst_val);
	    $dst = $dst_val . $dst_units;
	}

    }
    return $dst;
}

sub sensor_modules_selected() {
    my($self) = shift;
    my($rin) = @_;
    my($fd, $x, $i);
    my($selected) = 0;
    
    while( ($fd, $x) = each(%{$self->{sensor_modules_by_fd}}) ) {
	next unless(vec($rin, $fd, 1));

	$selected++;

	# debug
	#warn("sensor_modules_selected: pid=$x->{pid} fd=$fd path=$x->{path}");

	# read and buffer input, then break at newlines
	$x->{buf} ||= '';
	$i = $x->{io}->sysread($x->{buf}, 4096, length($x->{buf}));

	# debug
	#warn("sensor_modules_selected: pid=$x->{pid} fd=$fd i=$i \$!=$! buf=[$x->{buf}]");

	# if the read would block, ignore it
	if (!defined($i) && $! == EAGAIN) {
	    next;
	}

	if( !defined($i) || $i <= 0 ) {
	    $x->{proc}->kill(SIGTERM);
	    delete $self->{sensor_modules_by_fd}{$fd};

	    # if sbox script has exited, then don't write to it (e.g. legacy)
	    $self->{sbox} = undef if ( $x->{path} =~ /sbox$/ );
	    next;
	}

	# sensor readings are "name: value\n"
	# gps: 48'33.9981N 123'29.6186W 25M
	# gps-speed: 000.0knots
	# sbox: mode=00 fwversion=SBOX-011-03
	# sbox: input=0 input_mask=16 output=3 battery=11.4

	#warn("start buf:[$x->{buf}]\n") if ( length($x->{buf}) > 0 );

	# the ///m (join lines) causes the \s to match over newlines 
	# which we don't want!  ESPIN 17Nov05
	#while( $x->{buf} =~ s/^ *([^ \t]*): *([^ \t]*)(.*)\r?\n// ) 

	while( $x->{buf} =~ s/^ *([^ \t]*): *([^ \t]*)\n// )
	{

	    #my($key, $val, $rem) = ($1, $2, $3);
	    my($key, $val) = ($1, $2);
	    my($val_orig) = $val;

	    #debug
	    #warn("buf:[$x->{buf}]\n") if ( length($x->{buf}) > 0 );

	    # convert between knots/mph/kph and Celcius/Farenheit
	    $val =~ s/([.\d]+)(knots|kts|mph|kph)/$self->units_convert("$1$2")/ge;

	    # debug
	    #warn("sensor_modules_selected: key=$key val=$val val_orig=$val_orig");
	    # gps-speed sets speed_mph
	    if( ($key eq 'gps-speed') && ($val =~ /^([\d.]+)/) && ($1 > 0)  ) {
		# debug
		#warn("sensor_modules_selected: set speed_mph=$1");

		$self->sensor('speed_mph', $1);
	    }
	    elsif( $key =~ 'sbox') {
		$self->sensor_sbox($key, $val_orig);
	    }
	    else {
		# new 25Nov05
		$self->sensor($key, $val);
	    }
	}

	#warn("end buf:[$x->{buf}]\n") if ( length($x->{buf}) > 0 );
    }
    return $selected;
}

return 1 if caller();

#---------------------------------------------------------------------
# test program

use lib qw(..);
package GKGun::Test;
use base qw(GKGun);
use strict;

#override GKGun::sensor to print sensor values
sub sensor() {
    my($self) = shift;
    my($name, $val) = @_;
    print("sensor name=$name val=$val\n");
}


my($gun) = GKGun::Test->new();

$gun->{sensor_modules_dir} = $ARGV[0] if @ARGV;

$gun->sensor_modules_init();

while(1) {
    my($rin) = '';

    vec($rin, fileno(STDIN), 1) = 1;

    $rin = $gun->sensor_modules_select($rin);
    select($rin, undef, undef, undef);
    $gun->sensor_modules_selected($rin); # calls $gun->sensor($name, $val)

    if( vec($rin, fileno(STDIN), 1) ) {
	last;
    }
}

$gun->sensor_modules_fini();



