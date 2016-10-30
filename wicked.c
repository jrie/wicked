//------------------------------------------------------------------------------
// Author: theSplit @ www.ngb.to 31.07.2016
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

//------------------------------------------------------------------------------

// Switches
#define BEVERBOSE false
#define DEBUG true
#define SOURCEFILE "data/enwik8_small"
//#define SOURCEFILE "enwiki-20160720-pages-meta-current1.xml-p000000010p000030303"
#define DICTIONARYFILE "data/words.txt"
#define WIKITAGSFILE "data/wikitags.txt"
#define LINETOPROCESS 0

// Buffers
#define LINEBUFFERBASE 5120

// Counts of predefined const datatypes
#define FORMATS 8
#define ENTITIES 211
#define INDENTS 3
#define TEMPLATES 11
#define TAGTYPES 10
#define TAGCLOSINGS 3

//------------------------------------------------------------------------------

// Custom datatypes
#pragma pack()
typedef struct keyValuePair {
  char *key;
  char *value;
} keyValuePair;

#pragma pack()
typedef struct word {
  unsigned int position;
  unsigned int wordFileStart;
  char formatType;
  unsigned char preSpacesCount;
  unsigned char spacesCount;
  bool formatGroup;
  //char *data;
} word;

#pragma pack()
typedef struct entity {
  unsigned short translationIndex;
  unsigned int position;
  unsigned char preSpacesCount;
  unsigned char spacesCount;
  char formatType;
  char data[3];
  bool formatGroup;
} entity;

#pragma pack()
typedef struct wikiTag {
  unsigned char preSpacesCount;
  unsigned char spacesCount;
  unsigned int position;
  unsigned int wordCount;
  unsigned int wTagCount;
  unsigned int entityCount;
  unsigned int wikiTagFileIndex;
  char formatType;
  unsigned char tagType;
  bool formatGroup;
  //char *target;
  struct word *pipedWords;
  struct wikiTag *pipedTags;
  struct entity *pipedEntities;
} wikiTag;

#pragma pack()
typedef struct xmlNode {
  unsigned short indent;
  short lastAddedType; // 0 WORD, 1 WIKITAG, 2 ENTITY
  unsigned int keyValuePairs;
  unsigned int wordCount;
  unsigned int entityCount;
  unsigned int wTagCount;
  unsigned int start;
  unsigned int end;
  void *lastAddedElement;
  bool isDataNode;
  bool isClosed;
  char *name;
  struct keyValuePair *keyValues;
  struct word *words;
  struct entity *entities;
  struct wikiTag *wikiTags;
} xmlNode;

typedef struct xmlDataCollection {
  unsigned int count;
  unsigned int openNodeCount;
  unsigned int *openNodes;
  struct xmlNode *nodes;
} xmlDataCollection;

typedef struct collectionStatistics {
  unsigned int wordCount;
  unsigned int entityCount;
  unsigned int wikiTagCount;
  unsigned int keyCount;
  unsigned int valueCount;
  unsigned int byteWords;
  unsigned int byteEntites;
  unsigned int byteWikiTags;
  unsigned int byteWhitespace;
  unsigned int byteKeys;
  unsigned int byteValues;
  unsigned int bytePreWhiteSpace;
  unsigned int byteXMLsaved;
  unsigned int byteNewLine;
  unsigned int byteFormatting;
  unsigned int failedElements;
} collectionStatistics;

//------------------------------------------------------------------------------

typedef struct parserDataHandle {
  FILE* dictFile;
  FILE* dictReadFile;
  FILE* wtagFile;
  struct xmlDataCollection* xmlCollection;
  struct collectionStatistics* cData;
} parserDataHandle;

//------------------------------------------------------------------------------

/*
  Wikipedia data types and such

  See for wikitags
  https://en.wikipedia.org/wiki/Help:Wiki_markup
  https://en.wikipedia.org/wiki/Help:Cheatsheet

  Reference for templates:
  https://en.wikipedia.org/wiki/Help:Wiki_markup
*/

const char formatNames[FORMATS][15] = {
  "Bold + Italic",
  "Bold",
  "Italic",
  "Heading 6",
  "Heading 5",
  "Heading 4",
  "Heading 3",
  "Heading 2"
};

const char formats[FORMATS][7] = {
  "'''''", // bold + italic
  "'''", // bold
  "''", // italic
  "======", // Heading 6 - heading start
  "=====",
  "====",
  "===",
  "==" // Heading 2 - heading end
};

/*
  NOTE: Indents are online used at the beginning of the line!
*/
const char indents[INDENTS][2] = {
  "*", // List element, multiple levels, "**" Element of element
  "#", // Numbered list element, multiple levels = "##" "Element of element"
  ":", // "indent 1", multiple levels using ":::" = 3
};

const char templates[TEMPLATES][18] = {
  // NOTE: Should we cover template tags as well for shortening?
  "{{-",
  "{{align",
  "{{break",
  "{{clear",
  "{{float",
  "{{stack",
  "{{outdent",
  "{{plainlist",
  "{{fake heading",
  "{{endplainlist",
  "{{unbulleted list"
};

const char wikiTagNames[TAGTYPES][19] = {
  "Definition/Anchor",
  "Table",
  "Category",
  "Media type",
  "File type",
  "Image type",
  "Sound type",
  "Wiktionary",
  "Wikipedia",
  "Link"
};

const char tagTypes[TAGTYPES][15] = {
  // NOTE: Handle definitions after, in case we threat, templates too
  "{{", // Definition => {{Main|autistic savant}}
  "{|", // Table start => ! (headline), |- (seperation, row/border), | "entry/ column data"
  "[[category:",
  "[[media:", // Media types start
  "[[file:",
  "[[image:", // [[Image:LeoTolstoy.jpg|thumb|150px|[[Leo Tolstoy|Leo Tolstoy]] 1828-1910]]
  "[[sound:", // Media types end
  //"#REDIRECT", // Redirect #REDIRECT [[United States]] (article) --- #REDIRECT [[United States#History]] (section)
  "[[wiktionary:", // [[wiktionary:terrace|terrace]]s
  "[[wikipedia:", // [[Wikipedia:Nupedia and Wikipedia]]
  "[[" // Link => [[Autistic community#Declaration from the autism community|sent a letter to the United Nations]]
};

const char tagClosingsTypes[TAGCLOSINGS][3] = {
  "}}",
  "]]",
  "|}"
};

const char entities[ENTITIES][2][32] = {
  //Commercial symbols
  {"trade", "\xe2\x84\xa2"},
  {"copy", "\xc2\xa9"},
  {"reg", "\xc2\xae"},
  {"cent", "\xc2\xa2"},
  {"euro", "\xe2\x82\xac"},
  {"yen", "\xc2\xa5"},
  {"pound", "\xc2\xa3"},
  {"curren", "\xc2\xa4"},
  //Diacritical marks
  {"Agrave", "\xc3\x80"},
  {"Aacute", "\xc3\x81"},
  {"Acirc", "\xc3\x82"},
  {"Atilde", "\xc3\x83"},
  {"Auml", "\xc3\x84"},
  {"Aring", "\xc3\x85"},
  {"AElig", "\xc3\x86"},
  {"Ccedil", "\xc3\x87"},
  {"Egrave", "\xc3\x88"},
  {"Eacute", "\xc3\x89"},
  {"Ecirc", "\xc3\x8a"},
  {"Euml", "\xc3\x8b"},
  {"Igrave", "\xc3\x8c"},
  {"Iacute", "\xc3\x8d"},
  {"Icirc", "\xc3\x8e"},
  {"Iuml", "\xc3\x8f"},
  {"Ntilde", "\xc3\x91"},
  {"Ograve", "\xc3\x92"},
  {"Oacute", "\xc3\x93"},
  {"Ocirc", "\xc3\x94"},
  {"Otilde", "\xc3\x95"},
  {"Ouml", "\xc3\x96"},
  {"Oslash", "\xc3\x98"},
  {"OElig", "\xc5\x92"},
  {"Ugrave", "\xc3\x99"},
  {"Uacute", "\xc3\x9a"},
  {"Ucirc", "\xc3\x9b"},
  {"Uuml", "\xc3\x9c"},
  {"Yuml", "\xc5\xb8"},
  {"szlig", "\xc3\x9f"},
  {"agrave", "\xc3\xa0"},
  {"aacute", "\xc3\xa1"},
  {"acirc", "\xc3\xa2"},
  {"atilde", "\xc3\xa3"},
  {"auml", "\xc3\xa4"},
  {"aring", "\xc3\xa5"},
  {"aelig", "\xc3\xa6"},
  {"ccedil", "\xc3\xa7"},
  {"egrave", "\xc3\xa8"},
  {"eacute", "\xc3\xa9"},
  {"ecirc", "\xc3\xaa"},
  {"euml", "\xc3\xab"},
  {"igrave", "\xc3\xac"},
  {"iacute", "\xc3\xad"},
  {"icirc", "\xc3\xae"},
  {"iuml", "\xc3\xaf"},
  {"ntilde", "\xc3\xb1"},
  {"ograve", "\xc3\xb2"},
  {"oacute", "\xc3\xb3"},
  {"ocirc", "\xc3\xb4"},
  {"otilde", "\xc3\xb5"},
  {"ouml", "\xc3\xb6"},
  {"oslash", "\xc3\xb8"},
  {"oelig", "\xc5\x93"},
  {"ugrave", "\xc3\xb9"},
  {"uacute", "\xc3\xba"},
  {"ucirc", "\xc3\xbb"},
  {"uuml", "\xc3\xbc"},
  {"yuml", "\xc3\xbf"},
  //Greek characters
  {"alpha", "\xce\xb1"},
  {"beta", "\xce\xb2"},
  {"gamma", "\xce\xb3"},
  {"delta", "\xce\xb4"},
  {"epsilon", "\xce\xb5"},
  {"zeta", "\xce\xb6"},
  {"Alpha", "\xce\x91"},
  {"Beta", "\xce\x92"},
  {"Gamma", "\xce\x93"},
  {"Delta", "\xce\x94"},
  {"Epsilon", "\xce\x95"},
  {"Zeta", "\xce\x96"},
  {"eta", "\xce\xb7"},
  {"theta", "\xce\xb8"},
  {"iota", "\xce\xb9"},
  {"kappa", "\xce\xba"},
  {"lambda", "\xce\xbb"},
  {"mu", "\xce\xbc"},
  {"nu", "\xce\xbd"},
  {"Eta", "\xce\x97"},
  {"Theta", "\xce\x98"},
  {"Iota", "\xce\x99"},
  {"Kappa", "\xce\x9a"},
  {"Lambda", "\xce\x9b"},
  {"Mu", "\xce\x9c"},
  {"Nu", "\xce\x9d"},
  {"xi", "\xce\xbe"},
  {"omicron", "\xce\xbf"},
  {"pi", "\xcf\x80"},
  {"rho", "\xcf\x81"},
  {"sigma", "\xcf\x83"},
  {"sigmaf", "\xcf\x82"},
  {"Xi", "\xce\x9e"},
  {"Omicron", "\xce\x9f"},
  {"Pi", "\xce\xa0"},
  {"Rho", "\xce\xa1"},
  {"Sigma", "\xce\xa3"},
  {"tau", "\xcf\x84"},
  {"upsilon", "\xcf\x85"},
  {"phi", "\xcf\x86"},
  {"chi", "\xcf\x87"},
  {"psi", "\xcf\x88"},
  {"omega", "\xcf\x89"},
  {"Tau", "\xce\xa4"},
  {"Upsilon", "\xce\xa5"},
  {"Phi", "\xce\xa6"},
  {"Chi", "\xce\xa7"},
  {"Psi", "\xce\xa8"},
  {"Omega", "\xce\xa9"},
  //Mathematical characters and formulae
  {"int", "\xe2\x88\xab"},
  {"sum", "\xe2\x88\x91"},
  {"prod", "\xe2\x88\x8f"},
  {"radic", "\xe2\x88\x9a"},
  {"minus", "\xe2\x88\x92"},
  {"plusmn", "\xc2\xb1"},
  {"infin", "\xe2\x88\x9e"},
  {"asymp", "\xe2\x89\x88"},
  {"prop", "\xe2\x88\x9d"},
  {"equiv", "\xe2\x89\xa1"},
  {"ne", "\xe2\x89\xa0"},
  {"le", "\xe2\x89\xa4"},
  {"ge", "\xe2\x89\xa5"},
  {"times", "\xc3\x97"},
  {"middot", "\xc2\xb7"},
  {"divide", "\xc3\xb7"},
  {"part", "\xe2\x88\x82"},
  {"prime", "\xe2\x80\xb2"},
  {"Prime", "\xe2\x80\xb3"},
  {"nabla", "\xe2\x88\x87"},
  {"permil", "\xe2\x80\xb0"},
  {"deg", "\xc2\xb0"},
  {"there4", "\xe2\x88\xb4"},
  {"alefsym", "\xe2\x84\xb5"},
  {"oslash", "\xc3\xb8"},
  {"isin", "\xe2\x88\x88"},
  {"notin", "\xe2\x88\x89"},
  {"cap", "\xe2\x88\xa9"},
  {"cup", "\xe2\x88\xaa"},
  {"sub", "\xe2\x8a\x82"},
  {"sup", "\xe2\x8a\x83"},
  {"sube", "\xe2\x8a\x86"},
  {"supe", "\xe2\x8a\x87"},
  {"not", "\xc2\xac"},
  {"and", "\xe2\x88\xa7"},
  {"or", "\xe2\x88\xa8"},
  {"exist", "\xe2\x88\x83"},
  {"forall", "\xe2\x88\x80"},
  {"rArr", "\xe2\x87\x92"},
  {"lArr", "\xe2\x87\x90"},
  {"dArr", "\xe2\x87\x93"},
  {"uArr", "\xe2\x87\x91"},
  {"hArr", "\xe2\x87\x94"},
  {"rarr", "\xe2\x86\x92"},
  {"darr", "\xe2\x86\x93"},
  {"uarr", "\xe2\x86\x91"},
  {"larr", "\xe2\x86\x90"},
  {"harr", "\xe2\x86\x94"},
  //Predefined entities in XML
  {"quot", "\""},
  {"amp", "&"},
  {"apos", "'"},
  {"lt", "<"},
  {"gt", ">"},
  //Punctuation special characters
  {"iquest", "\xc2\xbf"},
  {"iexcl", "\xc2\xa1"},
  {"sect", "\xc2\xa7"},
  {"para", "\xc2\xb6"},
  {"dagger", "\xe2\x80\xa0"},
  {"Dagger", "\xe2\x80\xa1"},
  {"bull", "\xe2\x80\xa2"},
  {"ndash", "\xe2\x80\x93"},
  {"mdash", "\xe2\x80\x94"},
  {"lsaquo", "\xe2\x80\xb9"},
  {"rsaquo", "\xe2\x80\xba"},
  {"laquo", "\xc2\xab"},
  {"raquo", "\xc2\xbb"},
  {"lsquo", "\xe2\x80\x98"},
  {"rsquo", "\xe2\x80\x99"},
  {"ldquo", "\xe2\x80\x9c"},
  {"rdquo", "\xe2\x80\x9d"},
  {"apos", "'"},
  //Subscripts and superscripts
  {"#8320", "\xe2\x82\x80"},
  {"#8321", "\xe2\x82\x81"},
  {"#8322", "\xe2\x82\x82"},
  {"#8323", "\xe2\x82\x83"},
  {"#8324", "\xe2\x82\x84"},
  {"#8325", "\xe2\x82\x85"},
  {"#8326", "\xe2\x82\x86"},
  {"#8327", "\xe2\x82\x87"},
  {"#8328", "\xe2\x82\x88"},
  {"#8329", "\xe2\x82\x89"},
  {"#8304", "\xe2\x81\xb0"},
  {"sup1", "\xc2\xb9"},
  {"sup2", "\xc2\xb2"},
  {"sup3", "\xc2\xb3"},
  {"#8308", "\xe2\x81\xb4"},
  {"#8309", "\xe2\x81\xb5"},
  {"#8310", "\xe2\x81\xb6"},
  {"#8311", "\xe2\x81\xb7"},
  {"#8312", "\xe2\x81\xb8"},
  {"#8313", "\xe2\x81\xb9"},
  {"epsilon", "\xce\xb5"},
  {"times", "\xc3\x97"},
  {"minus", "\xe2\x88\x92"},
};

//------------------------------------------------------------------------------
// Function declarations
int parseXMLNode(const unsigned int, const unsigned int, const unsigned int, const char*, const struct parserDataHandle*);
int parseXMLData(const unsigned int, const unsigned int, const unsigned int, const char*, struct xmlNode*, const struct parserDataHandle*);

/*
  NOTE: First parameters of all three functions: elementType.
        elementType = 0 => xmlNode
        elementType = 1 => wikiTag
        void *element = xmlNode/wikiTag
*/
bool addWikiTag(short, void*, const char, const bool, const short, const unsigned char, const unsigned char, const unsigned int, const unsigned int, const char*, const struct parserDataHandle*);
bool addEntity(short, void*, const char, const bool, const unsigned char, unsigned const char, const unsigned int, const unsigned int, const char[], const struct parserDataHandle*);
bool addWord(const short, void*, const unsigned int, const char, const bool, const unsigned char, const unsigned char, const unsigned int, const unsigned int, const char*, const struct parserDataHandle*);

// Clean up functions
void freeXMLCollection(xmlDataCollection*);
void freeXMLCollectionTag(wikiTag*);

//------------------------------------------------------------------------------
// Main routine
int main(int argc, char *argv[]) {
  printf("[ INFO ] Starting parsing process on file \"%s\".\n", SOURCEFILE);

  FILE *inputFile = fopen(SOURCEFILE, "r");
  remove(DICTIONARYFILE);
  remove(WIKITAGSFILE);

  FILE *dictFile = fopen(DICTIONARYFILE, "w");
  FILE *wtagFile = fopen(WIKITAGSFILE, "w");
  FILE *dictReadFile = fopen(DICTIONARYFILE, "r");

  collectionStatistics cData = {0};

  if (inputFile == NULL) {
    printf("Cannot open file '%s' for reading.\n", SOURCEFILE);
    return 1;
  }

  time_t startTime = time(NULL);

  // Parser variables
  unsigned int lineBuffer = LINEBUFFERBASE;
  char *line = malloc(sizeof(char) * LINEBUFFERBASE);

  unsigned int currentLine = 1;
  unsigned int lineLength = 0;
  char tmpChar = '\0';

  bool nodeAdded = false;

  xmlDataCollection xmlCollection = {0, 0, NULL, NULL};

  parserDataHandle parserRunTimeData;
  parserRunTimeData.dictFile = dictFile;
  parserRunTimeData.wtagFile = wtagFile;
  parserRunTimeData.dictReadFile = dictReadFile;
  parserRunTimeData.xmlCollection = &xmlCollection;
  parserRunTimeData.cData = &cData;

  //----------------------------------------------------------------------------
  // Parser start
  while (tmpChar != -1) {
    lineLength = 0;

    if (LINETOPROCESS != 0 && currentLine > LINETOPROCESS) {
      break;
    }

    // Read a line from file
    do {
      tmpChar = fgetc(inputFile);
      //printf("%d\n", lineLength);
      line[lineLength] = tmpChar;
      ++lineLength;

      if (lineLength == lineBuffer) {
        lineBuffer += LINEBUFFERBASE;
        line = (char*) realloc(line, sizeof(char) * lineBuffer);
      }

    } while (tmpChar != '\n' && tmpChar != -1);

    ++cData.byteNewLine;

    line[lineLength] = '\0';

    if (line[0] != '\n') {
      // Find XML Tags on line
      if (strchr(line, '>') != NULL) {
        unsigned int xmlTagStart = 0;

        /*
        typedef struct parserData {
          FILE* dictFile;
          FILE* dictReadFile;
          FILE* wtagFile;
          struct xmlDataCollection* xmlCollection;
          struct collectionStatistics* statisticsData;
          struct collectionStatistics* cData;
        } parserData;

        typedef struct dataHandle {
          short elementType;
          short wikiTagType;
          void* element;
          unsigned int dataLength;
          unsigned int fileLine;
          unsigned int position;
          char formatType;
          bool formatGroup;
          unsigned char preSpacesCount;
          unsigned char spacesCount;
          char* data;

        } dataHandle;

        typedef struct xmlDataHandle {
          unsigned int startinPos;
          unsigned int lineLength;
          unsigned int currentLine;
          struct xmlNode* xmlTag;
          char *line;
        } xmlDataHandle;
        */

        if (line[0] == '<') {
          parseXMLNode(0, lineLength, currentLine, line, &parserRunTimeData);
          nodeAdded = true;
        } else {
          xmlTagStart = strchr(line, '<') - line;

          if (xmlTagStart > 0) {
            cData.bytePreWhiteSpace += xmlTagStart;
            parseXMLNode(xmlTagStart, lineLength, currentLine, line, &parserRunTimeData);
            nodeAdded = true;
          }
        }
      }

      if (!nodeAdded) {
        // Read in data line to current xmlNode
        parseXMLData(0, lineLength, currentLine, line, &xmlCollection.nodes[xmlCollection.count-1], &parserRunTimeData);
      }

      nodeAdded = false;
    } else {
      ++cData.byteNewLine;
    }

    ++currentLine;
    //printf("%d\n", currentLine);
  }

  long int duration = difftime(time(NULL), startTime);
  long int durHours = floor(duration / 3600);
  long int durMinutes = floor((duration % 3600) / 60);
  long int durSeconds = (duration % 3600) % 60;
  printf("\n\n[STATUS] RUN TIME FOR PARSING PROCESS: %ldh %ldm %lds\n", durHours, durMinutes, durSeconds);
  printf("[REPORT] PARSED LINES : %d | FAILED ELEMENTS: %d\n", currentLine, cData.failedElements);
  printf("[REPORT] FILE STATISTICS\nKEYS       : %16d [~ %.3lf MB]\nVALUES     : %16d [~ %.3lf MB]\nWORDS      : %16d [~ %.3lf MB]\nENTITIES   : %16d [~ %.3lf MB]\nWIKITAGS   : %16d [~ %.3lf MB]\nWHITESPACE : %16d [~ %.3lf MB]\nFORMATTING : [~ %.3lf MB]\n\nTOTAL COLLECTED DATA : ~%.3lf MB (~%.3lf MB xml name and closing)\n", cData.keyCount, cData.byteKeys / 1048576.0, cData.valueCount, cData.byteValues / 1048576.0, cData.wordCount, cData.byteWords / 1048576.0, cData.entityCount, cData.byteEntites / 1048576.0, cData.wikiTagCount, cData.byteWikiTags / 1048576.0, cData.byteWhitespace, cData.byteWhitespace / 1048576.0, cData.byteFormatting / 1048576.0, (cData.byteKeys + cData.byteValues + cData.byteWords + cData.byteEntites + cData.byteWikiTags + cData.byteWhitespace + cData.byteFormatting + cData.bytePreWhiteSpace + cData.byteNewLine + cData.byteXMLsaved) / 1048576.0, cData.byteXMLsaved / 1048576.0);
  printf("TOTAL FILE SIZE: ~%.3lf MB\n\n", ftell(inputFile) / 1048576.0);

  fclose(inputFile);
  fclose(dictFile);
  fclose(wtagFile);

  //----------------------------------------------------------------------------
  // Cleanup
  free(line);
  freeXMLCollection(&xmlCollection);
  return 0;
}

//------------------------------------------------------------------------------

int parseXMLNode(const unsigned int xmlTagStart, const unsigned int lineLength, const unsigned int currentLine, const char *line, const struct parserDataHandle* parserRunTimeData) {

  xmlDataCollection* xmlCollection = parserRunTimeData->xmlCollection;
  collectionStatistics* cData = parserRunTimeData->cData;

  //printf("in parse xmlNode: %d\n", currentLine);
  xmlCollection->nodes = (xmlNode*) realloc(xmlCollection->nodes, sizeof(xmlNode) * (xmlCollection->count + 1));
  xmlNode *xmlTag = &xmlCollection->nodes[xmlCollection->count];

  /*
  typedef struct xmlNode {
    unsigned short indent;
    short lastAddedType; // 0 WORD, 1 WIKITAG, 2 ENTITY
    unsigned int keyValuePairs;
    unsigned int wordCount;
    unsigned int entityCount;
    unsigned int wTagCount;
    unsigned int start;
    unsigned int end;
    void *lastAddedElement;
    bool isDataNode;
    bool isClosed;
    char *name;
    struct keyValuePair *keyValues;
    struct word *words;
    struct entity *entities;
    struct wikiTag *wikiTags;
  } xmlNode;
  */

  xmlTag->isClosed = false;
  xmlTag->isDataNode = false;
  xmlTag->indent = xmlTagStart;
  xmlTag->lastAddedType = -1; // 0 WORD, 1 WIKITAG, 2 ENTITY
  xmlTag->keyValuePairs = 0;
  xmlTag->wordCount = 0;
  xmlTag->entityCount = 0;
  xmlTag->wTagCount = 0;
  xmlTag->start = currentLine;
  xmlTag->end = 0;
  xmlTag->lastAddedElement = NULL;
  xmlTag->name = NULL;
  xmlTag->keyValues = NULL;
  xmlTag->words = NULL;
  xmlTag->entities = NULL;
  xmlTag->wikiTags = NULL;


  // Routine variables
  unsigned int readerPos = xmlTagStart + 1;

  char readIn = '\0';
  char readData[lineLength];
  int writerPos = 0;

  bool xmlHasName = false;
  bool isKey = false;
  bool isValue = false;

  bool nodeClosed = false;
  keyValuePair *xmlKeyValue = NULL;

  // Identify the xmlTag name and key and value pairs
  while (line[readerPos] != '>') {
    while (line[readerPos] != '>') {
      readIn = line[readerPos];

      if (!isValue) {
        if (readIn == ' ') {
          ++readerPos;
          ++cData->byteWhitespace;
          break;
        } else if (readIn == '=') {
          readerPos += 2;
          isKey = true;
          break;
        } else if (readIn == '/') {
          xmlTag->isClosed = true;
          xmlTag->end = currentLine;
          readerPos++;
          continue;
        }
      } else if (readIn == '"') {
        ++readerPos;
        break;
      }

      readData[writerPos] = readIn;
      ++writerPos;
      ++readerPos;
      continue;
    }

    if (writerPos != 0) {
      readData[writerPos] = '\0';
      if (!xmlHasName) {
        xmlTag->name = malloc(sizeof(char) * (writerPos + 1));
        strcpy(xmlTag->name, readData);

        cData->byteXMLsaved += ((writerPos) * 2) + 5;

        xmlHasName = true;
      } else if (isKey) {
        xmlTag->keyValues = (keyValuePair*) realloc(xmlTag->keyValues, sizeof(keyValuePair) * (xmlTag->keyValuePairs + 1) );

        xmlKeyValue = &xmlTag->keyValues[xmlTag->keyValuePairs];
        xmlKeyValue->value = NULL;

        xmlKeyValue->key = malloc(sizeof(char) * (writerPos + 1));
        strcpy(xmlKeyValue->key, readData);

        cData->byteKeys += writerPos;
        ++cData->keyCount;

        ++xmlTag->keyValuePairs;

        isKey = false;
        isValue = true;
      } else if (isValue) {
        xmlKeyValue->value = malloc(sizeof(char) * (writerPos + 1));
        strcpy(xmlKeyValue->value, readData);

        cData->byteValues += (writerPos+2);
        ++cData->valueCount;

        isValue = false;
      }

      writerPos = 0;
    }
  }

  //--------------------------------------------------------------------------
  // NOTE: Does adjust so we know if data follows the xmlTag
  ++readerPos;

  char *endPointer = strrchr(line, '<');
  unsigned int xmlEnd = endPointer != NULL ? endPointer - line : 0;

  if (readerPos > xmlEnd) {
    xmlEnd = lineLength-1;

    if (DEBUG) {
      printf("\n\n[DEBUG] RP: %8d | END: %8d | LINELENGTH: %d | NAME: %s\n", readerPos, xmlEnd, lineLength, xmlTag->name);
    }
  }

  //--------------------------------------------------------------------------

  if (xmlHasName) {
    ++xmlCollection->count;

    if (!xmlTag->isClosed) {
      xmlCollection->openNodes = (unsigned int*) realloc(xmlCollection->openNodes, sizeof(unsigned int) * (xmlCollection->openNodeCount + 1));
      xmlCollection->openNodes[xmlCollection->openNodeCount] = xmlCollection->count - 1;
      ++xmlCollection->openNodeCount;
    } else {
      nodeClosed = false;
      //printf("%s\n", readData);
      for (int i = xmlCollection->openNodeCount - 1; i != -1; --i) {
        xmlNode *openXMLNode = &xmlCollection->nodes[xmlCollection->openNodes[i]];
        if (strcmp(xmlTag->name, openXMLNode->name) == 0) {
          openXMLNode->isClosed = true;
          openXMLNode->end = currentLine;

          if (i < (xmlCollection->openNodeCount-1)) {
            memmove(&xmlCollection->openNodes[i], &xmlCollection->openNodes[i+1], (xmlCollection->openNodeCount-(i+1)) * sizeof(unsigned int));

            xmlCollection->openNodes = (unsigned int*) realloc(xmlCollection->openNodes, sizeof(unsigned int) * xmlCollection->openNodeCount);
            --xmlCollection->openNodeCount;
          }

          if (BEVERBOSE || DEBUG) {
            printf("[STATUS]  | LINE: %8d | %sNODE CLOSED: '%s'\n", currentLine, openXMLNode->isDataNode ? "DATA " : "", openXMLNode->name);
          }

          nodeClosed = true;

          break;
        }
      }


      if (nodeClosed) {
        if (xmlTagStart != 0) {
            unsigned int dataReaderPos = 0;
            bool readData = false;

            while (!readData) {
              switch (line[dataReaderPos]) {
                case ' ':
                  ++dataReaderPos;
                  continue;
                  break;
                case '<':
                  return 1;
                  break;
                default:
                  parseXMLData(dataReaderPos, xmlEnd, currentLine, line, xmlTag, parserRunTimeData);
                  readData = true;
                  break;
              }
            }
        }

        if (BEVERBOSE || DEBUG) {
          printf("[STATUS]  | LINE: %8d | %sNODE CLOSED: '%s'\n", currentLine, xmlTag->isDataNode ? "DATA " : "", xmlTag->name);
        }

        return 1;
      }

    }

    if (BEVERBOSE || DEBUG) {
      printf("\n\n[STATUS]  | LINE: %8d | %s%sNODE ADDED: '%s'\n", currentLine, xmlTag->isClosed ? "CLOSED " : "", readerPos < xmlEnd ? "DATA " : "", xmlTag->name);
      for (unsigned int i = 0; i < xmlTag->keyValuePairs; ++i) {
        printf("[STATUS]  | KEYVALUE: '%s' > '%s'\n", xmlTag->keyValues[i].key, xmlTag->keyValues[i].value);
      }
    }
  } else {
    return 0;
  }

  // Start process the data of the XML tag
  if (readerPos < xmlEnd) {
    xmlTag->isDataNode = true;
    parseXMLData(readerPos, xmlEnd, currentLine, line, xmlTag, parserRunTimeData);
  }

  return 1;
}

//------------------------------------------------------------------------------

int parseXMLData(unsigned int readerPos, const unsigned int lineLength, const unsigned int currentLine, const char* line, xmlNode *xmlTag, const struct parserDataHandle* parserRunTimeData) {

  collectionStatistics* cData = parserRunTimeData->cData;

  if (DEBUG) {
    printf("====================================================================\n");
    printf("[DEBUG] START PARSING DATA => TAG: '%s' | READERPOS: %d | LINE: %d\n", xmlTag->name, readerPos, currentLine);
  } else if (BEVERBOSE) {
    printf("====================================================================\n");
  }

  char readIn = '\0';
  char readData[lineLength];

  bool isAdded = false;
  bool createWord = false;
  unsigned char spacesCount = 0;
  unsigned char preSpacesCount = 0;
  unsigned int position = 0;

  char tmpChar = '\0';
  unsigned int writerPos = 0;

  char formatData[lineLength];
  unsigned short formatReaderPos = 0;
  unsigned short formatDataPos = 0;

  // WIKITAG variables
  short wikiTagType = -1;
  bool isWikiTag = false;
  unsigned short wikiTagDepth = 0;
  bool doCloseWikiTag = false;

  // FORMATs variables
  char formatType = -1;
  bool doCloseFormat = false;
  // NOTE: Do we need the formatting group switch?
  bool formatGroup = 0;

  bool isEntity = false;
  char entityBuffer[11] = "\0";
  unsigned short entityReadPos = 0;
  unsigned short entityWritePos = 0;
  readIn = '\0';
  //printf("in parese func, currentLine: %d\n", currentLine);

  //printf("line: %d\n", currentLine);
  while (readerPos <= lineLength) {

    if (readIn == '<') {
      break;
    }

    readIn = line[readerPos];

    // Escape before xml tag closings and such
    switch (readIn) {
      case '\n':
        if (writerPos != 0) {
          createWord = true;
        }
        break;
      case '\t':
        if (writerPos != 0) {
          createWord = true;
          break;
        }

        if (writerPos == 0) {
          preSpacesCount += 2;
        } else {
          spacesCount += 2;
        }

        cData->byteWhitespace += 2;

        ++readerPos;
        continue;
      case ' ':
        ++cData->byteWhitespace;
        if (isWikiTag) {
          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }

        if (writerPos != 0) {
          ++spacesCount;
          createWord = true;
        } else {
          ++preSpacesCount;
        }

        break;
      case '<':
        createWord = true;
        break;
      case '[':
      case '{':
        tmpChar = line[readerPos + 1];

        if (tmpChar == readIn) {
          if (!isWikiTag && writerPos != 0) {
            createWord = true;
            --readerPos;
            break;
          }

          ++wikiTagDepth;

          if (!isWikiTag) {

            formatReaderPos = readerPos;
            formatDataPos = 0;

            do {
              tmpChar = line[formatReaderPos];

              if (formatDataPos == 32 || tmpChar == '\0') {
                break;
              }

              formatData[formatDataPos] = tolower(tmpChar);

              ++formatDataPos;
              ++formatReaderPos;
            } while (tmpChar != ' ' && tmpChar != '|' && tmpChar != '\0');

            formatData[formatDataPos] = '\0';
            wikiTagType = -1;
            for (unsigned short i = 0; i < TAGTYPES; ++i) {
              if (strncmp(formatData, tagTypes[i], strlen(tagTypes[i])) == 0) {
                wikiTagType = i;
                isWikiTag = true;
                break;
              }
            }

            if (wikiTagType != -1) {
              readerPos += strlen(tagTypes[wikiTagType]);
            }

            formatDataPos = 0;
            formatData[0] = '\0';

            if (isWikiTag) {
              continue;
            }
          }
        }

        readData[writerPos] = readIn;
        ++writerPos;
        ++readerPos;
        continue;
        break;
      case ']':
      case '}':
        tmpChar = line[readerPos+1];

        if (tmpChar == readIn) {
          if (wikiTagDepth != 0) {
            --wikiTagDepth;
            ++readerPos;

            if (wikiTagDepth <= 0) {
              doCloseWikiTag = true;
              break;
            } else  if (isWikiTag) {
              readData[writerPos] = readIn;
              ++writerPos;
            }
          } else {
            if (writerPos != 0) {
              createWord = true;
              break;
            }

            ++readerPos;
            continue;
          }
        }

        readData[writerPos] = readIn;
        ++writerPos;

        ++readerPos;
        continue;
        break;
      case '=':
      case '\'':
        if (isWikiTag) {
          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }

        formatReaderPos = readerPos;
        formatDataPos = 0;

        while (line[formatReaderPos] == readIn) {
          formatData[formatDataPos] = readIn;
          ++formatDataPos;
          ++formatReaderPos;
        }

        if (readerPos == 0 && formatDataPos == (lineLength - 1)) {
          if (readIn == '=') {
            formatData[formatDataPos] = '\0';
            strcpy(readData, formatData);
            createWord = true;
            readerPos = formatReaderPos;
            break;
          } else if (xmlTag->lastAddedType == 1){
            readerPos = formatReaderPos;

            cData->byteFormatting += formatReaderPos;
            continue;
          }

          if (formatReaderPos == 1) {
            ++readerPos;
            createWord = true;
            break;
          }
        }

        if (formatDataPos == 1) {
          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        } else {
          formatData[formatDataPos] = '\0';

          for (unsigned short i = 0; i < FORMATS; ++i) {
            if (strcmp(formats[i], formatData) == 0) {
              if (formatType == -1) {
                formatType = i;
              } else if (formatType == i) {
                doCloseFormat = true;
              } else {
                if (DEBUG) {
                  printf("[DEBUG] Formatting does not match, forcing it to: %d > %d\n\n", formatType, i);
                }
                formatType = i;
              }

              readerPos += formatDataPos - 1;

              cData->byteFormatting += formatDataPos;
              break;
            }
          }
        }

        if (writerPos != 0) {
          createWord = true;
        }

        formatDataPos = 0;
        formatData[0] = '\0';
        break;
      case '&':
        if (isWikiTag) {
          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }

        entityReadPos = readerPos+1;
        entityWritePos = 0;

        isEntity = false;

        while(entityWritePos < 10 && line[entityReadPos] != ' ') {
          entityBuffer[entityWritePos] = line[entityReadPos];

          ++entityReadPos;
          ++entityWritePos;
          if (line[entityReadPos] == ';') {
            isEntity = true;
            break;
          }
        }

        entityBuffer[entityWritePos] = '\0';

        if (isEntity) {
          if (writerPos != 0) {
            createWord = true;
            isEntity = false;
            --readerPos;
          } else {
            readerPos += entityWritePos+1;
          }
        } else {
          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }

        break;
      default:
        readData[writerPos] = readIn;
        ++writerPos;
        ++readerPos;
        continue;
        break;
    }

    if (createWord || isEntity || doCloseWikiTag) {
      createWord = false;
      readData[writerPos] = '\0';

      if (DEBUG) {
        if (isWikiTag || isEntity || writerPos != 0) {
          printf("[DEBUG] LINE: %d | SPACING: %d/%d | WTD: %d | POS: %8d | READER: %8d | DATA: %5d", currentLine, preSpacesCount, spacesCount, wikiTagDepth, position, readerPos, writerPos);
        }
      }

      if (isWikiTag) {
        if (DEBUG) {
          printf(" | WIKITAG | DATA: \"%s\" '%s'", readData, wikiTagNames[wikiTagType]);
        } else if (BEVERBOSE) {
          printf("WIKITAG   | LINE: %8d | %8d | DATA: \"%s\" '%s'", currentLine, position, readData, wikiTagNames[wikiTagType]);
        }

        // TODO: Add wikiTag handling routine
        if (addWikiTag(0, xmlTag, formatType, formatGroup, wikiTagType, preSpacesCount, spacesCount, currentLine, position, readData, parserRunTimeData)) {
          isAdded = true;
          ++cData->wikiTagCount;
        }

      } else if (isEntity) {
        if (DEBUG) {
          printf(" |  ENTITY | DATA: \"%s\" ", entityBuffer);
        } else if (BEVERBOSE) {
          printf("ENTITY    | LINE: %8d | %8d | DATA: \"%s\"", currentLine, position, entityBuffer);
        }

        // TODO: Add entity handling routine
        if (addEntity(0, xmlTag, formatType, formatGroup, preSpacesCount, spacesCount, currentLine, position, entityBuffer, parserRunTimeData)) {
          isAdded = true;
          ++cData->entityCount;
        }

      } else if (writerPos != 0) {
        // Is regular word with read in data
        if (DEBUG) {
          printf(" |   WORD  | DATA: \"%s\" ", readData);
        } else if (BEVERBOSE) {
          printf("WORD      | LINE: %8d | %8d | DATA: \"%s\"", currentLine, position, readData);
        }

        // TODO: Think of "pre" and "/pre" as switch for nonformatting parts
        if (addWord(0, xmlTag, writerPos, formatType, formatGroup, preSpacesCount, spacesCount, currentLine, position, readData, parserRunTimeData)) {
          isAdded = true;
          ++cData->wordCount;
        }
      } else if (DEBUG) {
        // Having empty sequence here, something might be wrong!
        printf("[DEBUG] --- EMPTY SEQUENCE --->> LINE: %d | POS:%d | WTD: %d", currentLine, readerPos, wikiTagDepth);
      }

      //------------------------------------------------------------------------

      if (!isAdded && ((!isEntity && writerPos != 0) || (isEntity && entityWritePos != 0))) {
        ++cData->failedElements;
      }

      if (DEBUG || BEVERBOSE)  {
        if (formatType != -1) {
          printf(" << *%s", formatNames[(int)formatType]);
        }

        if (!isAdded)
          printf("     [FAILED]");

        printf("\n");
      }

      //------------------------------------------------------------------------

      // NOTE: RESETS
      isWikiTag = false;
      isEntity = false;
      doCloseWikiTag = false;
      isAdded = false;

      preSpacesCount = 0;
      spacesCount = 0;
      writerPos = 0;
      ++position;
    }

    if (doCloseFormat) {
      formatType = -1;
      doCloseFormat = false;
    }

    ++readerPos;
  }
  //printf("outside line: %d\n", currentLine);

  if (BEVERBOSE || DEBUG) {
    printf("--------------------------------------------------------------------\n");
  }

  return 1;
}

//------------------------------------------------------------------------------


bool addWikiTag(short elementType, void *element, const char formatType, const bool formatGroup, const short wikiTagType, const unsigned char preSpacesCount, const unsigned char spacesCount, const unsigned int fileLine, const unsigned int position, const char *readData, const struct parserDataHandle *parserRunTimeData) {

  collectionStatistics* cData = parserRunTimeData->cData;
  FILE* wtagFile = parserRunTimeData->wtagFile;

  /*
  typedef struct wikiTag {
    unsigned char preSpacesCount;
    unsigned char spacesCount;
    unsigned int position;
    unsigned int wordCount;
    unsigned int wTagCount;
    unsigned int entityCount;
    unsigned int WIKITAGSFILEIndex;
    char formatType;
    unsigned char tagType;
    bool formatGroup;
    //char *target;
    struct word *pipedWords;
    struct wikiTag *pipedTags;
    struct entity *pipedEntities;
  } wikiTag;
  */

  wikiTag *tag = NULL;
  xmlNode *xmlTag = NULL;
  wikiTag *parentTag = NULL;

  if (elementType == 0) {
    xmlTag = element;
    xmlTag->wikiTags = (wikiTag*) realloc(xmlTag->wikiTags, sizeof(wikiTag) * (xmlTag->wTagCount + 1));
    tag = &xmlTag->wikiTags[xmlTag->wTagCount];
  } else {
    parentTag = element;
    parentTag->pipedTags = (wikiTag*) realloc(parentTag->pipedTags, sizeof(wikiTag) * (parentTag->wTagCount + 1));
    tag = &parentTag->pipedTags[parentTag->wTagCount];
  }

  if (tag == NULL) {
    return 0;
  }

  tag->formatType = formatType;
  tag->tagType = wikiTagType;
  tag->formatGroup = formatGroup;
  tag->preSpacesCount = preSpacesCount;
  tag->spacesCount = spacesCount;
  tag->position = position;
  tag->wordCount = 0;
  tag->wTagCount = 0;
  tag->entityCount = 0;
  //tag->target = NULL;
  tag->wikiTagFileIndex = cData->wikiTagCount;
  tag->pipedWords = NULL;
  tag->pipedTags = NULL;
  tag->pipedEntities = NULL;

  unsigned int dataLength = strlen(readData);
  char *pipePlace = strchr(readData, '|');
  unsigned int pipeStart = pipePlace != NULL ? pipePlace - readData: 0;

  if (pipeStart != 0) {
    if (DEBUG)
      printf("\n[DEBUG] PARSING WIKITAG   => \"%s\" (%s) \n", readData, wikiTagNames[wikiTagType]);

    unsigned int readerPos = 0;
    unsigned int writerPos = 0;

    char readIn = '\0';
    char parserData[dataLength];

    char tmpChar = '\0';

    char formatData[64] = "\0";
    unsigned short formatDataPos = 0;
    unsigned short formatReaderPos = 0;
    short dataFormatType = -1;

    unsigned char spacesCount = 0;
    unsigned char preSpacesCount = 0;

    short tagType = -1;
    bool isWikiTag = false;
    unsigned short wikiTagDepth = 0;
    bool doCloseWikiTag = false;

    bool dataHasParentFormat = false;
    bool doCloseFormat = false;
    bool createWord = false;
    bool isAdded = false;

    bool isEntity = false;
    char entityBuffer[11] = "\0";
    unsigned short entityReadPos = 0;
    unsigned short entityWritePos = 0;

    while (readerPos <= dataLength) {
        if (readerPos < pipeStart) {
          parserData[writerPos] = readData[readerPos];
          ++writerPos;
          ++readerPos;
          continue;
        } else if (readerPos == pipeStart) {
          parserData[writerPos] = '\0';
          //tag->target = malloc(sizeof(char) * (writerPos+1) );
          //strcpy(tag->target, parserData);
          //fprintf(wtagFile, "%d|%d|%s\n", cData->wikiTagCount, writerPos - 1, parserData);
          fprintf(wtagFile, "%u|%d|%d|%d|%s\n", fileLine, wikiTagType, formatType, writerPos, parserData);

          cData->byteWikiTags += (writerPos-1);

          if (DEBUG)
            printf("[DEBUG] TARGET            => \"%s\"\n", parserData);

          writerPos = 0;
          ++readerPos;
          continue;
        } else if (readerPos > pipeStart) {
          readIn = readData[readerPos];

          switch (readIn) {
            case '\n':
              if (writerPos != 0) {
                createWord = true;
              }
              break;
            case '\t':
              if (writerPos != 0) {
                createWord = true;
                break;
              }

              if (writerPos == 0) {
                preSpacesCount += 2;
              } else {
                spacesCount += 2;
              }

              cData->byteWhitespace += 2;

              ++readerPos;
              continue;
            case ' ':
              ++cData->byteWhitespace;
            case '\0':
              if (isWikiTag && readIn != '\0') {
                parserData[writerPos] = readData[readerPos];
                ++writerPos;
                ++readerPos;
                continue;
              } else if (writerPos != 0) {
                createWord = true;
              }
              break;
            case '\'':
              formatDataPos = 0;
              formatReaderPos = readerPos;

              while (readData[formatReaderPos] == readIn) {
                formatData[formatDataPos] = readIn;
                ++formatDataPos;
                ++formatReaderPos;
              }

              if (formatDataPos == 1) {
                parserData[writerPos] = readData[readerPos];
                ++writerPos;
                break;
              }

              formatData[formatDataPos] = '\0';

              for (unsigned short i = 0; i < FORMATS; ++i) {
                if (strcmp(formats[i], formatData) == 0) {
                  if (dataFormatType == -1) {
                    dataFormatType = i;
                  } else if (dataFormatType == i) {
                    doCloseFormat = true;
                  } else {
                    // TODO: Add newst format handling code
                    dataHasParentFormat = true;
                  }

                  break;
                }
              }

              readerPos += formatDataPos - 1;
              cData->byteFormatting += formatDataPos - 1;
              break;
            case '|':
              ++cData->byteWikiTags;
              if (!isWikiTag) {
                createWord = true;
              }
              break;
            case '[':
            case '{':
              tmpChar = readData[readerPos+1];
              if (tmpChar == readIn) {
                if (!isWikiTag && writerPos != 0) {
                  createWord = true;
                  --readerPos;
                  break;
                }

                ++wikiTagDepth;

                if (!isWikiTag) {

                  formatReaderPos = readerPos;
                  formatDataPos = 0;

                  do {
                    tmpChar = readData[formatReaderPos];

                    if (formatDataPos == 32 || tmpChar == '\0') {
                      break;
                    }

                    formatData[formatDataPos] = tolower(tmpChar);

                    ++formatDataPos;
                    ++formatReaderPos;
                  } while (tmpChar != ' ' && tmpChar != '|');

                  formatData[formatDataPos] = '\0';

                  for (unsigned short i = 0; i < TAGTYPES; ++i) {
                    if (strncmp(formatData, tagTypes[i], strlen(tagTypes[i])) == 0) {
                      tagType = i;
                      isWikiTag = true;
                      break;
                    }
                  }

                  formatDataPos = 0;
                  formatData[0] = '\0';

                  if (isWikiTag) {
                    readerPos += strlen(tagTypes[tagType]);
                    continue;
                  } else {
                    parserData[writerPos] = readIn;
                    ++writerPos;
                    ++readerPos;
                    continue;
                  }
                }
              }

              parserData[writerPos] = readData[readerPos];
              ++writerPos;
              ++readerPos;
              continue;
              break;
            case ']':
            case '}':
              tmpChar = readData[readerPos+1];

              if (tmpChar == readIn) {
                if (wikiTagDepth != 0) {
                  --wikiTagDepth;
                  ++readerPos;

                  if (wikiTagDepth <= 0) {
                    doCloseWikiTag = true;
                    break;
                  } else  if (isWikiTag) {
                    parserData[writerPos] = readIn;
                    ++writerPos;
                  }
                } else {
                  if (writerPos != 0) {
                    createWord = true;
                    break;
                  }

                  ++readerPos;
                  continue;
                }
              }

              parserData[writerPos] = readIn;
              ++writerPos;
              ++readerPos;
              continue;
              break;
            case '&':
              if (isWikiTag) {
                parserData[writerPos] = readIn;
                ++writerPos;
                ++readerPos;
                continue;
              }

              entityReadPos = readerPos+1;
              entityWritePos = 0;

              isEntity = false;

              while(entityWritePos < 10 && readData[entityReadPos] != ' ') {
                entityBuffer[entityWritePos] = readData[entityReadPos];

                ++entityReadPos;
                ++entityWritePos;
                if (readData[entityReadPos] == ';') {
                  isEntity = true;
                  break;
                }
              }

              entityBuffer[entityWritePos] = '\0';

              if (isEntity) {
                if (writerPos != 0) {
                  createWord = true;
                  isEntity = false;
                  --readerPos;
                } else {
                  readerPos += entityWritePos+1;
                }
              } else {
                parserData[writerPos] = readIn;
                ++writerPos;
                ++readerPos;
                continue;
              }

              break;
            default:
              parserData[writerPos] = readData[readerPos];
              ++writerPos;
              ++readerPos;
              continue;
              break;
          }
        }


        if (doCloseFormat || createWord || isWikiTag || doCloseWikiTag || isEntity) {
          parserData[writerPos] = '\0';
          isAdded = false;

          if (isEntity) {
            if (DEBUG) {
              printf("[DEBUG] FOUND [ ENTITY  ] => \"%s\"", entityBuffer);

              if (dataFormatType != -1) {
                printf(" => %s", formatNames[dataFormatType]);
              }
            }

            tag->pipedEntities = (entity*) realloc(tag->pipedEntities, sizeof(entity) * (tag->entityCount+1));
            if (addEntity(1, tag, dataFormatType, formatGroup, preSpacesCount, spacesCount, fileLine, position, entityBuffer, parserRunTimeData)) {
              isAdded = true;
              ++cData->entityCount;
            }

          } else if (isWikiTag) {
            if (DEBUG) {
              printf("[DEBUG] FOUND [ WIKITAG ] => \"%s\" (%s)", parserData, wikiTagNames[tagType]);

              if (dataFormatType != -1) {
                printf(" => %s\n", formatNames[dataFormatType]);
              } else {
                printf("\n");
              }
            }

            tag->pipedTags = (wikiTag*) realloc(tag->pipedTags, sizeof(wikiTag) * (tag->wTagCount+1));
            if (addWikiTag(1, tag, dataFormatType, formatGroup, tagType, preSpacesCount, spacesCount, fileLine, position, parserData, parserRunTimeData)) {
              isAdded = true;
              ++cData->wikiTagCount;
            }

          } else {
            if (DEBUG) {
              printf("[DEBUG] FOUND [ WORD    ] => \"%s\"", parserData);

              if (dataFormatType != -1) {
                printf(" => %s", formatNames[dataFormatType]);
              }
            }

            tag->pipedWords = (word*) realloc(tag->pipedWords, sizeof(word) * (tag->wordCount+1));
            if (addWord(1, tag, writerPos, dataFormatType, formatGroup, preSpacesCount, spacesCount, fileLine, position, parserData, parserRunTimeData)) {
              isAdded = true;
              ++cData->wordCount;
            }

          }

          if (!isAdded && ((!isEntity && writerPos != 0) || (isEntity && entityWritePos != 0))) {
            ++cData->failedElements;
          }

          if (DEBUG) {
            if (!isAdded) {
              printf(" [FAILED]");
            }

            printf("\n");
          }

          writerPos = 0;
          createWord = false;
          isWikiTag = false;
          isEntity = false;

          if (doCloseWikiTag) {
            tagType = -1;
            doCloseWikiTag = false;
          }

          if (doCloseFormat) {
            dataFormatType = -1;
            doCloseFormat = false;
          }

        }

        ++readerPos;
    }

    if (DEBUG) {
      printf("\n");
    }

  } else {
    if (DEBUG)
      printf("\n[DEBUG] ADDING WIKITAG  : \"%s\" (%s)", readData, wikiTagNames[wikiTagType]);

    //tag->target = malloc(sizeof(char) * (dataLength + 1));
    //strcpy(tag->target, readData);
    //fprintf(wtagFile, "%d|%d|%d|%d|%s\n", cData->wikiTagCount, dataLength, wikiTagType, formatType, readData);
    fprintf(wtagFile, "%u|%d|%d|%d|%s\n", fileLine, wikiTagType, formatType, dataLength, readData);

    cData->byteWikiTags += dataLength;
  }

  if (elementType == 0) {
    ++xmlTag->wTagCount;
    xmlTag->lastAddedType = 1;
    xmlTag->lastAddedElement = &tag;
  } else {
    ++parentTag->wTagCount;
  }

  cData->byteWikiTags += strlen(tagTypes[wikiTagType])+2;

  // 0 WORD, 1 WIKITAG, 2 ENTITY

  return 1;
}

//------------------------------------------------------------------------------


bool addEntity(short elementType, void *element, const char formatType, const bool formatGroup, const unsigned char preSpacesCount, const unsigned char spacesCount, const unsigned int fileLine, const unsigned int position, const char entityBuffer[], const struct parserDataHandle* parserRunTimeData) {
  /*
  typedef struct entity {
    short formatType;
    unsigned short formatGroup;
    unsigned short preSpacesCount;
    unsigned short spacesCount;
    //unsigned short translationIndex;
    unsigned int position;
    char data[3];
  } entity;
  */

  collectionStatistics* cData = parserRunTimeData->cData;

  for (unsigned short i = 0; i < ENTITIES; ++i) {
    if (strcmp(entityBuffer, entities[i][0]) == 0) {
      entity *tagEntity = NULL;
      xmlNode *xmlTag = NULL;
      wikiTag *tag = NULL;

      if (elementType == 0) {
        xmlTag = element;
        xmlTag->entities = (entity*) realloc(xmlTag->entities, sizeof(entity) * (xmlTag->entityCount + 1));
        tagEntity = &xmlTag->entities[xmlTag->entityCount];
      } else if (elementType == 1) {
        tag = element;
        tag->pipedEntities = (entity*) realloc(tag->pipedEntities, sizeof(entity) * (tag->entityCount + 1));
        tagEntity = &tag->pipedEntities[tag->entityCount];
      }

      if (tagEntity == NULL) {
        return 0;
      }

      tagEntity->formatType = formatType;
      tagEntity->formatGroup = formatGroup;
      tagEntity->preSpacesCount = preSpacesCount;
      tagEntity->spacesCount = spacesCount;
      tagEntity->position = position;
      tagEntity->translationIndex = i;
      strcpy(tagEntity->data, entities[i][1]);
      cData->byteEntites += strlen(entities[i][0])+2;

      if (elementType == 0) {
        ++xmlTag->entityCount;
        xmlTag->lastAddedType = 2;
        xmlTag->lastAddedElement = &tagEntity;
      } else {
        ++tag->entityCount;
      }

      return 1;
      break;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------


bool addWord(const short elementType, void *element, const unsigned int dataLength, const char formatType, const bool formatGroup, const unsigned char preSpacesCount, const unsigned char spacesCount, const unsigned int fileLine, const unsigned int position, const char *readData, const struct parserDataHandle* parserRunTimeData) {
  /*
  typedef struct word {
    unsigned int position;
    unsigned int wordFileStart;
    char formatType;
    unsigned char preSpacesCount;
    unsigned char spacesCount;
    bool formatGroup;
    //char *data;
  } word;
  */

  collectionStatistics* cData = parserRunTimeData->cData;
  FILE* dictFile = parserRunTimeData->dictFile;

  word *tagWord = NULL;
  xmlNode *xmlTag = NULL;
  wikiTag *tag = NULL;

  if (elementType == 0) {
    xmlTag = element;
    xmlTag->words = (word*) realloc(xmlTag->words, sizeof(word) * (xmlTag->wordCount + 1));
    tagWord = &xmlTag->words[xmlTag->wordCount];
  } else if (elementType == 1) {
    tag = element;
    tag->pipedWords = (word*) realloc(tag->pipedWords, sizeof(word) * (tag->wordCount + 1));
    tagWord = &tag->pipedWords[tag->wordCount];
  }

  if (tagWord == NULL) {
    return 0;
  }

  tagWord->formatType = formatType;
  tagWord->formatGroup = formatGroup;
  tagWord->preSpacesCount = preSpacesCount;
  tagWord->spacesCount = spacesCount;
  tagWord->position = position;

  //rewind(dictReadFile);

  bool isIncluded = false;
  /*
  // ReEnable to add dictionary file comparison
  unsigned int *fileIndex = malloc(sizeof(unsigned int));
  unsigned int *wordLength = malloc(sizeof(unsigned int));
  char fileWord[1024];
  unsigned int fileLine = 0;
  printf("[STATUS] Checking word: %d\n", wordCount);
  while (fscanf(dictReadFile, "%u|%s\n", wordLength, fileWord) == 2) {
    ++fileLine;
    if (*wordLength == dataLength) {
      fileWord[*wordLength+1] = '\0';

      if (strcmp(fileWord, readData) == 0) {
        tagWord->wordFileStart = *fileIndex;
        //printf("[STATUS] Found at index: %d\n", *fileIndex);
        isIncluded = true;

        break;
      }
    }
  }
  */


  if (!isIncluded) {
    // Part of file comparions
    /*
    ++fileLine;
    tagWord->wordFileStart = fileLine;
    */
    fprintf(dictFile, "%u|%d|%d|%s\n", fileLine, dataLength, formatType, readData);
    //fflush(dictFile);
  }

  /*
  // Part of file comparions
  free(fileIndex);
  free(wordLength);
  */

  //tagWord->data = malloc(sizeof(char) * (dataLength+1));
  //strcpy(tagWord->data, readData);

  cData->byteWords += dataLength;

  if (elementType == 0) {
    ++xmlTag->wordCount;
    xmlTag->lastAddedType = 0;
    xmlTag->lastAddedElement = &tagWord;
  } else {
    ++tag->wordCount;
  }
  return 1;
}

//------------------------------------------------------------------------------



void freeXMLCollection(xmlDataCollection *xmlCollection) {

  xmlNode *xmlTag = NULL;

  for (unsigned int i = 0; i < xmlCollection->count; ++i) {
      xmlTag = &xmlCollection->nodes[i];

      for (unsigned int j = 0; j < xmlTag->keyValuePairs; ++j) {
          free(xmlTag->keyValues[j].key);
          free(xmlTag->keyValues[j].value);
      }

      for (unsigned int j = 0; j < xmlTag->wordCount; ++j) {
        //free(xmlTag->words[j].data);
      }
      for (unsigned int j = 0; j < xmlTag->wTagCount; ++j) {
          freeXMLCollectionTag(&xmlTag->wikiTags[j]);
      }

      free(xmlTag->name);
      free(xmlTag->keyValues);
      free(xmlTag->words);
      free(xmlTag->entities);
      free(xmlTag->wikiTags);
  }

  free(xmlCollection->nodes);
  free(xmlCollection->openNodes);

  if (DEBUG) {
    printf("[DEBUG] Successfully cleaned up xmlCollection...\n");
  }

  return;
}

void freeXMLCollectionTag(wikiTag *wTag) {
  /*
  typedef struct wikiTag {
    short formatType;
    unsigned short tagType;
    unsigned short formatGroup;
    unsigned short preSpacesCount;
    unsigned short spacesCount;
    unsigned int position;
    unsigned int wordCount;
    unsigned int wTagCount;
    unsigned int entityCount;
    char *target;
    struct word *pipedWords;
    struct wikiTag *pipedTags;
    struct entity *pipedEntities;
  } wikiTag;
  */

  for (unsigned int i = 0; i < wTag->wordCount; ++i) {
    //free(wTag->pipedWords[i].data);
  }

  for (unsigned int i = 0; i < wTag->wTagCount; ++i) {
    freeXMLCollectionTag(&wTag->pipedTags[i]);
  }

  //free(wTag->target);
  free(wTag->pipedWords);
  free(wTag->pipedTags);
  free(wTag->pipedEntities);

  return;
}
