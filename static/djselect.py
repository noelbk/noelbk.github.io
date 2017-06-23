#! python
"""
djselect - get Django Model objects from a cooked SQL select
statement, like HQL for Django

:Author: Noel Burton-Krahn <noel@burton-krahn.com>
:Created: Mar 2, 2007
:License: Public domain.
"""

import sys
import re

def get_parent_frame():
    """
    Return your caller's frame.  Used to look up objects in your caller's
    namespace.
    """
    parent_frame = None
    try:
        # throw an exception to get the stack frame
        1/0 
    except:
        tb = sys.exc_info()[2]
        parent_frame = tb.tb_frame.f_back.f_back
    return parent_frame

def frame_lookup(frame, key):
    """
    Lookup key in the frame's local and global dicts.
    """
    for h in (frame.f_locals, frame.f_globals):
        if key in h: return h[key]
    raise AttributeError("couldn't find %s" % (key,))

class DjSelect:
    """
    return Django Model objects from parsed SQL.

    Synopsis
    --------

        select = DjSelect('''
            from Pet:pet
              left join Owner:owner
                on owner.id = owner
            where pet.age > %s
            ''')
        for pet, owner in select.execute(age):
            print("pet=%s, owner=%s" % (pet, owner))


    Description
    -----------

    This class parses cooked SQL statements and returns tuples of
    Django Model objects.

    Model tables are identified in SQL as 'Model:alias', where 'Model'
    is the name of a django.db.models.Model class in the caller's
    namespace.  'alias' is used to refer to the model table in the
    rest of the SQL statement.  Each tuple fetched by execute() will
    have one model object per alias, returned in the order they appear
    in the SQL.

    If there is exactly one Model in the return tuple, the Model
    itself will be returned, instead of a tuple with just one element.

    After identifying 'Model:alias' constructs, this will search for
    'alias.attribute'. 'attribute' will be replaced by the Model
    field's SQL name.  An exception will be thrown if 'attribute' does
    not exist in Model.

    For example:
        sql='''from Model1:model1 Model2 model2
            where model1.attr1=%s and model2.attr2=%s'''
        for model1, model2 in DjSelect(sql).execute(val1, val2):
            ...
       
    If 'alias' is of the form 'alias.attribute', then the model object
    will be stuffed into an attribute of another Model named 'alias',
    instead of in the return tuple.

    Example: returns one model object with the second stuffed into
      model.property:
    
        sql='''
            from Model1:model1, Model2 model1.property
            where model1.attr1=%s and model1.property.attr2=%s
            '''
        for model1, in DjSelect(sql).execute(val1, val2):
            # The second Model2 object is in model1.property 
            ...

    The SQL parser adds a select clause to the SQL with the explicit
    names of all Model columns.  Use 'Model!alias' to identify a model
    that will not be added to the select clause.  That's to support
    nested sub-selects
    
    Example: use Model!alias for subselects
    
        sql='''
            from Model1:model1
            where model1.prop in (select prop from Model2!prop)
            '''
        for model1 in DjSelect(sql).execute(val1, val2):
            # Model2 is not selected, because it's declared with Model2!prop
            ...

    Examples
    --------
    
      The examples below illustrate how the SQL processing works.
      Once you've got your SQL, you can use it in DJSelect like so::

        for model1, model2 in DjSelect(sql).execute(params):
            print("fetched django objects: %s" % ((model1,model2)))
    
      SQL to return a tuple (owner, pet) per row::
        
        from Owner:owner, Pet:pet
        where owner.id = pet.owner
        order by owner.name, pet.name
        
      SQL to return one Pet object per row, with the (possibly null)
      owner stuffed into the pet.owner attribute::
      
        from Pet:pet
          left join Owner:pet.owner
            on pet.owner.id = pet.owner
        where pet.birthday < ?
        order by owner.name, pet.name
        
    Public Members
    --------------

    DjSelect(sql): parse and prepare a sql select statement.  Parsed
      SQL is available in the DJSelect objects's 'sql' parameter.
    
    instance.execute(params): Execute parsed sql with parameters.
      Calls django.db.connection.cursor().execute(sql, params)

    instance.sql: Parsed SQL to be sent to connection.execute()

    Limitations
    -----------

    It would be nice to select other attributes into returned models,
    like this:

        select("now() as model.now from Model:model")

    The SQL parser can't handle joined select statements like 'UNION',
    etc.

    The parsed SQL is sent raw to your database.  There is no
    cross-database translation.

    See also
    --------

      - django.db.models
      - django.db.connection
      - http://www.djangoproject.com/
    
    """

    # regexps to parse for Model:alias and alias patterns.
    re_model_table = re.compile(r"(?P<model>[.\w]+)(?P<noselect>[!:])(?P<alias>[.\w]+)")
    re_table_alias = re.compile(r"(?P<alias>[.\w]+)\.(?P<attr>\w+)")
    
    class DjSelectModelTable:
        """
        Represents one Model table in a select clause.
        """
        re_not_alias_char = re.compile(r"[^\w]")
        
        def __init__(self, model_name, alias, caller_frame):
            self.model_name = model_name
            self.alias = alias
            self.caller_frame = caller_frame
            self.column_start = 0
            
            self.sql_alias = self.re_not_alias_char.sub("__", self.alias)
            self.model = frame_lookup(self.caller_frame, self.model_name)
            self.meta = self.model.objects.model._meta
            self.db_table = self.meta.db_table
            self.fields = [f for f in self.meta.fields] #  if not f.rel]

    def __init__(self, sql):
        """
        find Model:alias and alias.attr patterns in sql, and replace
        with proper sql references.  Remembers the Model tables selected.
        """
        from django.db import backend
        
        self.caller_frame = get_parent_frame()
        self.column_start = 0
        self.table_by_alias = {}
        self.table_inorder = []
        
        self.sql_orig = sql
        self.sql = sql
        # find all the tables that map to a Model object.  They're
        # named like Model:alias
        self.sql = self.re_model_table.sub(self.match_model_table, self.sql)

        # find all instances of Model table aliases, and replace them
        # with aliasses that are more SQL friendly.
        self.sql = self.re_table_alias.sub(self.match_table_alias, self.sql)

        # add a select clause that selects all fields in Model tables
        sql_select = ""
        for table in self.table_inorder:
            if len(sql_select) > 0: sql_select += ",\n"
            sql_select += ",\n".join([backend.quote_name(table.sql_alias)
                                     + "."
                                     + backend.quote_name(f.column)
                                     for f in table.fields]);
            
        self.sql = "SELECT\n" + sql_select + "\n" + self.sql
        
        # debug
        #print("DjSelect.__init__: sql=%s" % (self.sql,))
        
    def match_model_table(self, match):
        """
        first pass: find all Model tables like Model:name
        """
        from django.db import backend
        noselect = match.group("noselect")
        if noselect == ':':
            noselect = False
        model = match.group("model")
        alias = match.group("alias")
        if self.table_by_alias.has_key(alias):
            raise Exception("duplicate alias: %s" %(alias,))
        table = self.DjSelectModelTable(model, alias, self.caller_frame)
        table.noselect = noselect
        self.table_by_alias[alias] = table

        if not noselect:
            table.column_start += self.column_start
            self.column_start += len(table.fields)
            self.table_inorder.append(table)
            
        return(backend.quote_name(table.db_table)
               + " "
               + backend.quote_name(table.sql_alias))

    def match_table_alias(self, match):
        """
        second pass: replace references to Model tables with their sql
        alias
        """
        from django.db import backend

        alias = match.group("alias")
        attr = match.group("attr")

        if not alias in self.table_by_alias:
            return match.group(0)

        table = self.table_by_alias[alias]
        field = table.meta.get_field(attr)

        return( backend.quote_name(table.sql_alias)
                + "."
                + backend.quote_name(field.column) )

    def execute(self, *params):
        """
        Execute select with arguments.  Calls cursor.execute(params)
        then iterates over cursor.fetchall(), yielding a tuple of
        Model objects for each row.  If the tuple has one element, it
        returns the element instead of a tuple of length 1.
        """
        from django.db import connection
        cursor = connection.cursor()
        
        # debug
        #print("DjSelect.execute sql=%s params=%s" % (self.sql, params))
        
        cursor.execute(self.sql, params)
        for row in cursor.fetchall():
            # debug
            #print("fetched row=%s" % (row,));

            table_list = []
            table_by_alias = {}
            subtables = {}
            
            for table in self.table_inorder:
                h = {}
                for f, val in zip(table.fields, row[table.column_start:]):
                    if f is table.meta.pk and val == None:
                        h = None
                        break
                    h[f.attname] = val

                # debug
                #print("making model from h=%s" % (h,));
                
                model = h and table.model(**h) or None

                # either model goes in table_list, or model gets
                # stuffed into a model already in table_list
                alias = table.alias
                if "." in alias:
                    subtables[alias] = model
                else:
                    table_by_alias[alias] = model
                    table_list.append(model)

                for alias in subtables:
                    model_name, attr = alias.split(".", 2)

                    # debug
                    #print("setting subtable alias=%s model_name=%s attr=%s"
                    #      % (alias, model_name, attr));
                     
                    model = table_by_alias[model_name]
                    setattr(model, attr, subtables[alias])
                    
            # debug
            #print("yielding: %s" % (table_list,))

            if len(table_list) == 1:
                yield table_list[0]
            else:
                yield table_list
    

if __name__ == '__main__':
    import os
    import sys
    os.environ['DJANGO_SETTINGS_MODULE']='settings'
    sys.path.append('..')
    from django.db import models
    from django.contrib.auth.models import *

    # test by selecting form a real Django model
    sel = DjSelect("""
            FROM Message:message
              LEFT JOIN User:message.user
                ON message.user.id = message.user
            WHERE message.message = %s
              AND message.user.id IN (SELECT user2.id FROM User!user2)
            ORDER BY message.user.username,
              message.message
             
              """)
    print("sql=%s" % (sel.sql,))
