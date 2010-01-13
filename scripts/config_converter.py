import sys
import struct
import xml.sax
import xml.sax.handler


class Attribute :
    NUMERIC = 0
    ASCII = 1
    BOOL = 2
    BINARY = 3

    TYPE_NAME_MAP = {
        "numeric" : NUMERIC,
        "ascii" : ASCII,
        "bool" : BOOL,
        "binary" : BINARY
    }

    def __init__( self, name, type, value ) :
        self.name = name
        self.type = type

        if type == self.NUMERIC or type == self.BOOL :
            self.value = int( value )
        else :
            self.value = value

    def getName( self ) :
        return self.name

    def getType( self ) :
        return self.type

    def getValue( self ) :
        return self.value

    def getSizeRequest( self ) :
        size = 0

        size += 1 # attribute type

        if self.type == self.NUMERIC :
            size += 8 # 8 byte for int64_t type
        elif self.type == self.ASCII :
            size += 4 # 4 byte for the size of the ascii string
            size += 8 # 8 byte for an offset to the data section
        elif self.type == self.BOOL :
            size += 1 # 1 byte for the boolean value
        elif self.type == self.BINARY :
            size += 4 # 4 byte for the size
            size += 8 # 8 byte for the data offset

        return size


class Node :
    TYPE_NODE = 1
    TYPE_ATTRIBUTE = 2

    def __init__( self, name, parent = None ) :
        self.name = name
        self.parent = parent
        self.children = []
        self.attributes = []

    def addChild( self, child ) :
        self.children += [ child ]

    def addAttribute( self, attrib ) :
        self.attributes += [ attrib ]

    def getName( self ) :
        return self.name

    def getParent( self ) :
        return self.parent

    def getChildren( self ) :
        return self.children

    def getAttributes( self ) :
        return self.attributes

    def getSizeRequest( self, recursive = True ) :
        size = 0

        size += 4 # children + attribute count

        for attr in self.attributes :
            size += 1 # type
            size += 4 # name length
            size += len( attr.getName() ) # attribute name
            size += attr.getSizeRequest()

        for node in self.children :
            size += 1 # type
            size += 4 # name length
            size += len( node.getName() ) # node name
            size += 8 # offset to the node

            if recursive :
                size += node.getSizeRequest()

        return size


class ConfigHandler( xml.sax.handler.ContentHandler ) :
    def __init__( self, root ) :
        self.current = root

    def startElement( self, name, attributes ) :
        if name == "node" :
            child = Node( attributes[ "name" ], self.current )
            self.current.addChild( child )
            self.current = child
        elif name == "attrib" :
            attrib = Attribute( attributes[ "name" ],
                                Attribute.TYPE_NAME_MAP[ attributes[ "type" ] ],
                                attributes[ "value" ] )
            self.current.addAttribute( attrib )

    def characters( self, data ) :
        pass

    def endElement(self, name):
        if name == "node" :
            self.current = self.current.getParent()


class BinaryWriter :
    def __init__( self, root ) :
        self.root = root
        self.file = None
        self.file_position = 0
        self.data_position = root.getSizeRequest()

    def writeFile( self, filename ) :
        self.file = open( filename, "wb" )
        self._writeNode( self.root )
        self.file.close()

    def _writeNode( self, node ) :
        children = node.getChildren()
        attributes = node.getAttributes()

        self.file_position += node.getSizeRequest( False )
        self._writeInt32( len( children ) + len( attributes ) )

        for child in children :
            self._writeInt8( Node.TYPE_NODE )
            self._writeInt32( len( child.getName() ) )
            self._writeString( child.getName() )
            self._writeInt64( self.file_position )

            current_position = self.file.tell()
            self.file.seek( self.file_position )
            self._writeNode( child )
            self.file.seek( current_position )

        for attrib in attributes :
            self._writeInt8( Node.TYPE_ATTRIBUTE )
            self._writeInt32( len( attrib.getName() ) )
            self._writeString( attrib.getName() )

            type = attrib.getType()
            self._writeInt8( type )

            if type == Attribute.NUMERIC :
                self._writeInt64( attrib.getValue() )
            elif type == Attribute.ASCII :
                self._writeInt32( len( attrib.getValue() ) )
                self._writeInt64( self.data_position )

                current_position = self.file.tell()
                self.file.seek( self.data_position )
                self._writeString( attrib.getValue() )
                self.file.seek( current_position )

                self.data_position += len( attrib.getValue() )
            elif type == Attribute.BOOL :
                self._wrintInt8( attrib.getValue() )
            elif type == Attribute.BINARY :
                bin = open( attrib.getValue(), "r" )
                binData = bin.read()
                bin.close()

                self._writeInt32( len( binData ) )
                self._writeInt64( self.data_position )

                current_position = self.file.tell()
                self.file.seek( self.data_position )
                self._writeString( binData )
                self.file.seek( current_position )

                self.data_position += len( binData )

    def _writeInt8( self, value ) :
        self.file.write( struct.pack( "B", value ) )

    def _writeInt32( self, value ) :
        self.file.write( struct.pack( "I", value ) )

    def _writeInt64( self, value ) :
        self.file.write( struct.pack( "Q", value ) )

    def _writeString( self, value ) :
        self.file.write( value )


if len( sys.argv ) != 3 :
    print "%s input_xml output_binary" % ( sys.argv[ 0 ] )
    sys.exit( 0 )

root = Node( "root" )

parser = xml.sax.make_parser()
handler = ConfigHandler( root )
parser.setContentHandler(handler)
parser.parse( sys.argv[1] )

writer = BinaryWriter( root.getChildren()[0] )
writer.writeFile( sys.argv[2] )
