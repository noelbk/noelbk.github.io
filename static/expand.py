#! /usr/bin/env python
"""
NAME

expand - expand $-expressions in strings, a la Perl

EXAMPLE

  Regular $variable interpolation
  
    >>> name="World"
    >>> xs("Hello $name")
    'Hello World'

  Debugging $variable interpolation with file:line location and
  variable names
  
    >>> xd("Hello $name")
    "<doctest __main__[2]>:1: Hello name='World'"

MOTIVATION

Why not just use % operator as in "Hello %s" % 'world'?  This is more
concise, especially when you have a long list of args for the % operator.

  log.debug(xd("got message $message from $user while doing $op"))

AUTHOR

Noel Burton-Krahn <noel@burton-krahn.com>

"""

import os, sys, re
import pprint
strx_re = re.compile(r'\$(?P<expr>'
                       + r'\$'
                       + r'|\w+(?:[.\w]+)*'
                       + r'|{(?:(?P<opts>\w+):)?(?P<expr2>[^}]+)?}'
                       + r')')


def xs(s, verbose=False, location=False, caller=None):
    """
Expands $-expressions in strings like Perl or sh.

      $$                   - literal "$"

      $expression          - eval expression (can contain dots)
      
      ${[opts:]expression} - eval expression, can be any expression
                             that doesn't contain a "}"

      opts can be:
        p - pretty print expr
        r - call repr(expr)
        v - prepend expansion with "expr=" and use repr(expr)

        by default, format with str(expr)

Throws exception if variable is not defined or expression fails

If verbose is true, prefix each expression like "expr=$expr"

if location is true, print the caller's function and line number

xs usually prints results with str, but you can use ${r:} or
${p:} to format results with repr() or pprint()

    >>> import datetime
    >>> dt = datetime.datetime(2011, 01, 02, 03, 04, 05)

By default, xs formats with str():

    >>> xs("expanded: $dt")
    'expanded: 2011-01-02 03:04:05'

The 'r' option formats with repr():

    >>> xs("expanded: ${r:dt}")
    'expanded: datetime.datetime(2011, 1, 2, 3, 4, 5)'

The 'v' option prepends the results "expr=" and formats with repr:

    >>> xs("expanded: ${sv:dt}")
    'expanded: dt=2011-01-02 03:04:05'

The 'p' option formats results with pprint.pformat:

    >>> a=range(10)
    >>> xs("expanded: ${p:a}")
    'expanded: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]'

xs recognizes special names like "function" and "file"

    >>> def test_expandfunc():
    ...     return xs("expanded func: $function $file")
    >>> test_expandfunc()
    'expanded func: test_expandfunc <doctest __main__.xs[7]>'

if location is True, print caller's file anf line number:

    >>> def test_location():
    ...     name="World"
    ...     return xs("expanded $name", location=True)
    >>> test_location()
    '<doctest __main__.xs[9]>:3: expanded World'

    """
    if not caller:
        caller = sys._getframe(1)
    globals = caller.f_globals
    locals = caller.f_locals

    def replace(m):
        expr = m.group('expr2') or m.group('expr')
        if expr == "$": 
            return "$"

        opt_repr = False
        opts = m.group('opts') or ''
        opt_verbose = 'v' in opts or verbose
        if opt_verbose:
            opt_repr = True
        opt_pprint = 'p' in opts
        if 'r' in opts:
            opt_repr = True
        if 's' in opts:
            opt_repr = False
        

        s = ""
        if opt_verbose:
            s += expr + "="

        # expand expr, handling special names like function, file, and lineno
        if expr == 'file':
            ret = caller.f_code.co_filename
        elif expr == 'function':
            ret = caller.f_code.co_name
        elif expr == 'lineno':
            ret = caller.f_code.co_firstlineno
        else:
            ret = eval(expr, globals, locals)

        # format expr
        if opt_pprint:
            s += pprint.pformat(ret)
        elif opt_repr:
            s += repr(ret)
        else:
            s += str(ret)
        return s

    ret = ""

    if location:
        ret += "%s:%d: " % (os.path.basename(caller.f_code.co_filename), caller.f_lineno)
        
    ret += strx_re.sub(replace, s)

    return ret

def class_repr(cls, attrs):
    return "%s(%s)" % (cls.__class__.__name__, 
                       (" ".join([ n + "=" + repr(getattr(cls, n)) for n in attrs ])))

def exprint(msg):
    """print the current file and line number, and expand msg"""
    caller = sys._getframe(1)
    print("%s:%d: %s" % 
          (caller.f_code.co_filename, caller.f_lineno, 
           xs(msg, verbose=True, globals=caller.f_globals, locals=caller.f_locals)))

def xd(s):
    """
    shortcut for xs(s, verbose=True, location=True)

    >>> x=dict(a=1)
    >>> xd("$x")
    "<doctest __main__.xd[1]>:1: x={'a': 1}"
    
    """
    return(xs(s, verbose=True, location=True, caller=sys._getframe(1)))
    
def allvars(caller=None):
    """
    return a single dictionary of globals() and locals() merged
    """
    if not caller:
        caller = sys._getframe(1)
    globals = caller.f_globals
    locals = caller.f_locals
    m = {}
    for d in globals, locals:
        m.update(d)
    return m
   
if __name__ == "__main__":
    print "Running doctest..."
    import doctest
    (failure_count, test_count) = doctest.testmod()
    if not failure_count:
        print "ok"
    else:
        print "failed"


