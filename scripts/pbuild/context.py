import re
import os
import functions
import definitions

class BuildContext :
    def __init__( self ) :
        self.targets = []
        self.default_target = None
        self.definition_manager = definitions.DefinitionManager()
        self.function_handler = functions.FunctionHandler()

    def add_definition( self, definition ) :
        self.definition_manager.add_definition( definition )

    def remove_definition( self, name ) :
        self.definition_manager.remove_definition( name )

    def add_target( self, target ) :
        self.targets.append( target )

    def get_target( self, name, private_allowed = False ) :
        for target in self.targets :
            if target.get_name() == name and ( private_allowed or not target.is_private() ) :
                return target

        return None

    def get_definition( self, name ) :
        return self.definition_manager.get_definition( name )

    def get_default_target( self ) :
        return self.default_target

    def set_default_target( self, target ) :
        self.default_target = target

    def replace_definitions( self, text ) :
        while True :
            start = text.find( "${" )

            if start == -1 :
                break

            end = text.find( "}", start + 2 )

            if end == -1 :
                break;

            def_text = text[ start:end + 1 ]
            def_name = definitions.extract_definition_name( def_text )
            definition = self.get_definition( def_name )

            if definition == None :
                text = text[ :start ] + text[ end + 1: ]
            else :
                text = text[ :start ] + definition.to_string() + text[ end + 1: ]

        return text

    def replace_definitions_in_array( self, array ) :
        new_array = []

        for item in array :
            new_array.append( self.replace_definitions( item ) )

        return new_array

    def replace_wildcards( self, text ) :
        pos = text.find( "*" )

        if pos == -1 :
            return [ text ]

        # Wildcard allowed only in filenames, check it

        dir_pos = text.rfind( "/" )

        if dir_pos > pos :
            return [ text ]

        directory = text[ 0:dir_pos ]
        pattern = text[ dir_pos + 1: ]

        pattern = pattern.replace( ".", "\\." )
        pattern = pattern.replace( "*", ".*" )
        mask = re.compile( pattern )

        entries = os.listdir( directory )

        result = []

        for entry in entries :
            if mask.match( entry ) :
                result.append( directory + os.sep + entry )

        return result

    def handle_everything( self, text ) :
        text = self.replace_definitions( text )
        return self.function_handler.handle_functions( text )
