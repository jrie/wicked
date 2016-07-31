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
#define beVerbose false
#define debug false
#define fileToParse "data/enwik8_small"
//#define fileToParse "enwiki-20160720-pages-meta-current1.xml-p000000010p000030303"
#define dictionaryFile "data/words.txt"
#define wikiTagFile "data/wikitags.txt"
#define linesToProcess 0

// Buffers
#define lineBufferBase 5120

// Counts of predefined const datatypes
#define _formats 8
#define _entities 205
#define _indents 3
#define _templates 11
#define _tagTypes 10
#define _tagClosings 3

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

//------------------------------------------------------------------------------

/*
  Wikipedia data types and such

  See for wikitags
  https://en.wikipedia.org/wiki/Help:Wiki_markup
  https://en.wikipedia.org/wiki/Help:Cheatsheet

  Reference for templates:
  https://en.wikipedia.org/wiki/Help:Wiki_markup
*/

const char formatNames[_formats][15] = {
  "Bold + Italic",
  "Bold",
  "Italic",
  "Heading 6",
  "Heading 5",
  "Heading 4",
  "Heading 3",
  "Heading 2"
};

const char formats[_formats][7] = {
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
const char indents[_indents][2] = {
  "*", // List element, multiple levels, "**" Element of element
  "#", // Numbered list element, multiple levels = "##" "Element of element"
  ":", // "indent 1", multiple levels using ":::" = 3
};

const char templates[_templates][18] = {
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

const char wikiTagNames[_tagTypes][19] = {
  "Definition/Anchor",
  "Table",
  "Category"
  "Media type",
  "File type",
  "Image type",
  "Sound type",
  "Wiktionary",
  "Wikipedia",
  "Link"
};

const char tagTypes[_tagTypes][15] = {
  // NOTE: Handle definitions after, in case we threat, templates too
  "{{", // Definition => {{Main|autistic savant}}
  "{|", // Table start => ! (headline), |- (seperation, row/border), | "entry/ column data"
  "[[Category:",
  "[[media:", // Media types start
  "[[file:",
  "[[image:", // [[Image:LeoTolstoy.jpg|thumb|150px|[[Leo Tolstoy|Leo Tolstoy]] 1828-1910]]
  "[[sound:", // Media types end
  //"#REDIRECT", // Redirect #REDIRECT [[United States]] (article) --- #REDIRECT [[United States#History]] (section)
  "[[wiktionary:", // [[wiktionary:terrace|terrace]]s
  "[[wikipedia:", // [[Wikipedia:Nupedia and Wikipedia]]
  "[[" // Link => [[Autistic community#Declaration from the autism community|sent a letter to the United Nations]]
};

const char tagClosingsTypes[_tagClosings][3] = {
  "}}",
  "]]",
  "|}"
};

const char entities[_entities][2][8] = {
  //Total count: 205
  //Commercial symbols
  {"trade", "™"},
  {"copy", "©"},
  {"reg", "®"},
  {"cent", "¢"},
  {"euro", "€"},
  {"yen", "¥"},
  {"pound", "£"},
  {"curren", "¤"},
  //Diacritical marks
  {"Agrave", "À"},
  {"Aacute", "Á"},
  {"Acirc", "Â"},
  {"Atilde", "Ã"},
  {"Auml", "Ä"},
  {"Aring", "Å"},
  {"AElig", "Æ"},
  {"Ccedil", "Ç"},
  {"Egrave", "È"},
  {"Eacute", "É"},
  {"Ecirc", "Ê"},
  {"Euml", "Ë"},
  {"Igrave", "Ì"},
  {"Iacute", "Í"},
  {"Icirc", "Î"},
  {"Iuml", "Ï"},
  {"Ntilde", "Ñ"},
  {"Ograve", "Ò"},
  {"Oacute", "Ó"},
  {"Ocirc", "Ô"},
  {"Otilde", "Õ"},
  {"Ouml", "Ö"},
  {"Oslash", "Ø"},
  {"OElig", "Œ"},
  {"Ugrave", "Ù"},
  {"Uacute", "Ú"},
  {"Ucirc", "Û"},
  {"Uuml", "Ü"},
  {"Yuml", "Ÿ"},
  {"szlig", "ß"},
  {"agrave", "à"},
  {"aacute", "á"},
  {"acirc", "â"},
  {"atilde", "ã"},
  {"auml", "ä"},
  {"aring", "å"},
  {"aelig", "æ"},
  {"ccedil", "ç"},
  {"egrave", "è"},
  {"eacute", "é"},
  {"ecirc", "ê"},
  {"euml", "ë"},
  {"igrave", "ì"},
  {"iacute", "í"},
  {"icirc", "î"},
  {"iuml", "ï"},
  {"ntilde", "ñ"},
  {"ograve", "ò"},
  {"oacute", "ó"},
  {"ocirc", "ô"},
  {"otilde", "õ"},
  {"ouml", "ö"},
  {"oslash", "ø"},
  {"oelig", "œ"},
  {"ugrave", "ù"},
  {"uacute", "ú"},
  {"ucirc", "û"},
  {"uuml", "ü"},
  {"yuml", "ÿ"},
  //Greek characters
  {"alpha", "α"},
  {"beta", "β"},
  {"gamma", "γ"},
  {"delta", "δ"},
  {"epsilon", "ε"},
  {"zeta", "ζ"},
  {"Alpha", "Α"},
  {"Beta", "Β"},
  {"Gamma", "Γ"},
  {"Delta", "Δ"},
  {"Epsilon", "Ε"},
  {"Zeta", "Ζ"},
  {"eta", "η"},
  {"theta", "θ"},
  {"iota", "ι"},
  {"kappa", "κ"},
  {"lambda", "λ"},
  {"mu", "μ"},
  {"nu", "ν"},
  {"Eta", "Η"},
  {"Theta", "Θ"},
  {"Iota", "Ι"},
  {"Kappa", "Κ"},
  {"Lambda", "Λ"},
  {"Mu", "Μ"},
  {"Nu", "Ν"},
  {"xi", "ξ"},
  {"omicron", "ο"},
  {"pi", "π"},
  {"rho", "ρ"},
  {"sigma", "σ"},
  {"sigmaf", "ς"},
  {"Xi", "Ξ"},
  {"Omicron", "Ο"},
  {"Pi", "Π"},
  {"Rho", "Ρ"},
  {"Sigma", "Σ"},
  {"tau", "τ"},
  {"upsilon", "υ"},
  {"phi", "φ"},
  {"chi", "χ"},
  {"psi", "ψ"},
  {"omega", "ω"},
  {"Tau", "Τ"},
  {"Upsilon", "Υ"},
  {"Phi", "Φ"},
  {"Chi", "Χ"},
  {"Psi", "Ψ"},
  {"Omega", "Ω"},
  //Mathematical characters and formulae
  {"int", "∫"},
  {"sum", "∑"},
  {"prod", "∏"},
  {"radic", "√"},
  {"minus", "−"},
  {"plusmn", "±"},
  {"infin", "∞"},
  {"asymp", "≈"},
  {"prop", "∝"},
  {"equiv", "≡"},
  {"ne", "≠"},
  {"le", "≤"},
  {"ge", "≥"},
  {"times", "×"},
  {"middot", "·"},
  {"divide", "÷"},
  {"part", "∂"},
  {"prime", "′"},
  {"Prime", "″"},
  {"nabla", "∇"},
  {"permil", "‰"},
  {"deg", "°"},
  {"there4", "∴"},
  {"alefsym", "ℵ"},
  {"isin", "∈"},
  {"notin", "∉"},
  {"cap", "∩"},
  {"cup", "∪"},
  {"sub", "⊂"},
  {"sup", "⊃"},
  {"sube", "⊆"},
  {"supe", "⊇"},
  {"not", "¬"},
  {"and", "∧"},
  {"or", "∨"},
  {"exist", "∃"},
  {"forall", "∀"},
  {"rArr", "⇒"},
  {"lArr", "⇐"},
  {"dArr", "⇓"},
  {"uArr", "⇑"},
  {"hArr", "⇔"},
  {"rarr", "→"},
  {"darr", "↓"},
  {"uarr", "↑"},
  {"larr", "←"},
  {"harr", "↔"},
  //Predefined entities in XML
  {"quot", "\""},
  {"amp", "&"},
  {"apos", "'"},
  {"lt", "<"},
  {"gt", ">"},
  //Punctuation special characters
  {"iquest", "¿"},
  {"iexcl", "¡"},
  {"sect", "§"},
  {"para", "¶"},
  {"dagger", "†"},
  {"Dagger", "‡"},
  {"bull", "•"},
  {"ndash", "–"},
  {"mdash", "—"},
  {"lsaquo", "‹"},
  {"rsaquo", "›"},
  {"laquo", "«"},
  {"raquo", "»"},
  {"lsquo", "‘"},
  {"rsquo", "’"},
  {"ldquo", "“"},
  {"rdquo", "”"},
  //Subscripts and superscripts
  {"#8320", "₀"},
  {"#8321", "₁"},
  {"#8322", "₂"},
  {"#8323", "₃"},
  {"#8324", "₄"},
  {"#8325", "₅"},
  {"#8326", "₆"},
  {"#8327", "₇"},
  {"#8328", "₈"},
  {"#8329", "₉"},
  {"#8304", "⁰"},
  {"sup1", "¹"},
  {"sup2", "²"},
  {"sup3", "³"},
  {"#8308", "⁴"},
  {"#8309", "⁵"},
  {"#8310", "⁶"},
  {"#8311", "⁷"},
  {"#8312", "⁸"},
  {"#8313", "⁹"}
};

//------------------------------------------------------------------------------
// Function declarations
int parseXMLNode(const unsigned int, const unsigned int, const unsigned int, const char*, xmlDataCollection*, FILE*, FILE*, FILE*);
int parseXMLData(const unsigned int, const unsigned int, const unsigned int, const char*, xmlNode*, FILE*, FILE*, FILE*);

/*
  NOTE: First parameters of all three functions: elementType.
        elementType = 0 => xmlNode
        elementType = 1 => wikiTag
        void *element = xmlNode/wikiTag
*/
bool addWikiTag(short, void*, const char, const bool, const short, const unsigned char, const unsigned char, const unsigned int, const char*, FILE*, FILE*, FILE*);
bool addEntity(short, void*, const char, const bool, const unsigned char, unsigned const char, const unsigned int, const char[]);
bool addWord(const short, void*, const unsigned int, const char, const bool, const unsigned char, const unsigned char, const unsigned int, const char*, FILE*, FILE*);

// Clean up functions
void freeXMLCollection(xmlDataCollection*);
void freeXMLCollectionTag(wikiTag*);

unsigned int wordCount = 0;
unsigned int entityCount = 0;
unsigned int wikiTagCount = 0;
unsigned int keyCount = 0;
unsigned int valueCount = 0;
unsigned int byteWords = 0;
unsigned int byteEntites = 0;
unsigned int byteWikiTags = 0;
unsigned int byteWhitespace = 0;
unsigned int byteKeys = 0;
unsigned int byteValues = 0;
unsigned int bytePreWhiteSpace = 0;
unsigned int byteXMLsaved = 0;
unsigned int byteNewLine = 0;
unsigned int byteFormatting = 0;
unsigned int failedElements = 0;

//------------------------------------------------------------------------------
// Main routine
int main(int argc, char *argv[]) {
  printf("[ INFO ] Starting parsing process on file \"%s\".\n", fileToParse);

  FILE *inputFile = fopen(fileToParse, "r");
  remove(dictionaryFile);
  remove(wikiTagFile);

  FILE *dictFile = fopen(dictionaryFile, "w");
  FILE *wtagFile = fopen(wikiTagFile, "w");
  FILE *dictReadFile = fopen(dictionaryFile, "r");

  if (inputFile == NULL) {
    printf("Cannot open file '%s' for reading.\n", fileToParse);
    return 1;
  }

  time_t startTime = time(NULL);

  // Parser variables
  unsigned int lineBuffer = lineBufferBase;
  char *line = malloc(sizeof(char) * lineBufferBase);

  unsigned int currentLine = 1;
  unsigned int lineLength = 0;
  char tmpChar = '\0';

  bool nodeAdded = false;

  xmlDataCollection xmlCollection = {0, 0, NULL, NULL};

  //----------------------------------------------------------------------------
  // Parser start
  while (!feof(inputFile)) {
    lineLength = 0;

    if (linesToProcess != 0 && currentLine > linesToProcess) {
      break;
    }

    // Read a line from file
    do {
      tmpChar = fgetc(inputFile);
      //printf("%d\n", lineLength);
      line[lineLength] = tmpChar;
      ++lineLength;

      if (lineLength == lineBuffer) {
        lineBuffer += lineBufferBase;
        line = (char*) realloc(line, sizeof(char) * lineBuffer);
      }

    } while (tmpChar != '\n' && !feof(inputFile));
    ++byteNewLine;
    //++lineLength;

    if (feof(inputFile)) {
      break;
    }

    line[lineLength] = '\0';

    if (line[0] != '\n') {
      // Find XML Tags on line
      if (strchr(line, '>') != NULL) {
        unsigned int xmlTagStart = 0;

        if (line[0] == '<') {
          parseXMLNode(0, lineLength, currentLine, line, &xmlCollection, dictFile, wtagFile, dictReadFile);
          nodeAdded = true;
        } else {
          xmlTagStart = strchr(line, '<') - line;

          if (xmlTagStart > 0) {
            bytePreWhiteSpace += xmlTagStart;
            parseXMLNode(xmlTagStart, lineLength, currentLine, line, &xmlCollection, dictFile, wtagFile, dictReadFile);
            nodeAdded = true;
          }
        }
      }

      if (!nodeAdded) {
        // Read in data line to current xmlNode
        parseXMLData(0, lineLength, currentLine, line, &xmlCollection.nodes[xmlCollection.count-1], dictFile, wtagFile, dictReadFile);
      }

      nodeAdded = false;
    } else {
      ++byteNewLine;
    }

    ++currentLine;
    //printf("%d\n", currentLine);
  }

  fclose(inputFile);
  fclose(dictFile);
  fclose(wtagFile);

  long int duration = difftime(time(NULL), startTime);
  long int durHours = floor(duration / 3600);
  long int durMinutes = floor((duration % 3600) / 60);
  long int durSeconds = (duration % 3600) % 60;
  printf("\n\n[STATUS] RUN TIME FOR PARSING PROCESS: %ldh %ldm %lds\n", durHours, durMinutes, durSeconds);
  printf("[REPORT] PARSED LINES : %d | FAILED ELEMENTS: %d\n", currentLine, failedElements);
  printf("[REPORT] FILE STATISTICS\nKEYS       : %16d [~ %.3lf MB]\nVALUES     : %16d [~ %.3lf MB]\nWORDS      : %16d [~ %.3lf MB]\nENTITIES   : %16d [~ %.3lf MB]\nWIKITAGS   : %16d [~ %.3lf MB]\nWHITESPACE : %16d [~ %.3lf MB]\nFORMATTING : [~ %.3lf MB]\n\nTOTAL COLLECTED DATA : ~%.3lf MB + (~ %.3lf MB pre whitespace + ~ %.3lf MB xml name and closing + ~ %.3lf MB newline char)\n", keyCount, byteKeys / 1048576.0, valueCount, byteValues / 1048576.0, wordCount, byteWords / 1048576.0, entityCount, byteEntites / 1048576.0, wikiTagCount, byteWikiTags / 1048576.0, byteWhitespace, byteWhitespace / 1048576.0, byteFormatting / 1048576.0, (byteKeys + byteValues + byteWords + byteEntites + byteWikiTags + byteWhitespace + byteFormatting) / 1048576.0, bytePreWhiteSpace / 1048576.0, byteXMLsaved / 1048576.0, byteNewLine / 1048576.0);
  //----------------------------------------------------------------------------
  // Cleanup
  free(line);
  freeXMLCollection(&xmlCollection);
  return 0;
}

//------------------------------------------------------------------------------



int parseXMLNode(const unsigned int xmlTagStart, const unsigned int lineLength, const unsigned int currentLine, const char *line, xmlDataCollection *xmlCollection, FILE *dictFile, FILE *wtagFile, FILE *dictReadFile) {
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
          ++byteWhitespace;
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

        byteXMLsaved += ((writerPos) * 2) + 5;

        xmlHasName = true;
      } else if (isKey) {
        xmlTag->keyValues = (keyValuePair*) realloc(xmlTag->keyValues, sizeof(keyValuePair) * (xmlTag->keyValuePairs + 1) );

        xmlKeyValue = &xmlTag->keyValues[xmlTag->keyValuePairs];
        xmlKeyValue->value = NULL;

        xmlKeyValue->key = malloc(sizeof(char) * (writerPos + 1));
        strcpy(xmlKeyValue->key, readData);

        byteKeys += writerPos;
        ++keyCount;

        ++xmlTag->keyValuePairs;

        isKey = false;
        isValue = true;
      } else if (isValue) {
        xmlKeyValue->value = malloc(sizeof(char) * (writerPos + 1));
        strcpy(xmlKeyValue->value, readData);

        byteValues += (writerPos+2);
        ++valueCount;

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

    if (debug) {
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

          if (beVerbose || debug) {
            printf("[STATUS]  | LINE: %8d | %sNODE CLOSED: '%s'\n", currentLine, openXMLNode->isDataNode ? "DATA " : "", openXMLNode->name);
          }

          nodeClosed = true;

          break;
        }
      }

      if (nodeClosed)
        return 1;
    }

    if (beVerbose || debug) {
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
    parseXMLData(readerPos, xmlEnd, currentLine, line, xmlTag, dictFile, wtagFile, dictReadFile);
  }

  return 1;
}

//------------------------------------------------------------------------------

int parseXMLData(unsigned int readerPos, const unsigned int lineLength, const unsigned int currentLine, const char* line, xmlNode *xmlTag, FILE *dictFile, FILE *wtagFile, FILE *dictReadFile) {
  if (debug) {
    printf("====================================================================\n");
    printf("[DEBUG] START PARSING DATA => TAG: '%s' | READERPOS: %d | LINE: %d\n", xmlTag->name, readerPos, currentLine);
  } else if (beVerbose) {
    printf("====================================================================\n");
  }

  //printf("isnide parseXMLData, line: %d\n", currentLine);

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
  //printf("in parese func, currentLine: %d\n", currentLine);

  //printf("line: %d\n", currentLine);
  while (readerPos <= lineLength) {
    readIn = line[readerPos];

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

        byteWhitespace += 2;

        ++readerPos;
        continue;
      case ' ':
        ++byteWhitespace;
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
          ++wikiTagDepth;

          if (!isWikiTag) {

            formatReaderPos = readerPos;
            formatDataPos = 0;

            do {
              tmpChar = line[formatReaderPos];

              if (formatDataPos == 64 || (formatReaderPos > 1 && tmpChar == ':') || tmpChar == '\0') {
                break;
              }

              formatData[formatDataPos] = tolower(tmpChar);

              ++formatDataPos;
              ++formatReaderPos;
            } while (tmpChar != ' ' && tmpChar != '|' && tmpChar != '\0');

            formatData[formatDataPos] = '\0';

            for (unsigned short i = 0; i < _tagTypes; ++i) {
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

            byteFormatting += formatReaderPos;
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

          for (unsigned short i = 0; i < _formats; ++i) {
            if (strcmp(formats[i], formatData) == 0) {
              if (formatType == -1) {
                formatType = i;
              } else if (formatType == i) {
                doCloseFormat = true;
              } else {
                if (debug) {
                  printf("[DEBUG] Formatting does not match, forcing it to: %d > %d\n\n", formatType, i);
                }
                formatType = i;
              }

              readerPos += formatDataPos-1;

              byteFormatting += formatDataPos;
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

    if (doCloseFormat) {
      formatType = -1;
      doCloseFormat = false;
    }

    if (createWord || isEntity || doCloseWikiTag) {
      createWord = false;
      readData[writerPos] = '\0';

      if (debug) {
        if (isWikiTag || isEntity || writerPos != 0) {
          printf("[DEBUG] LINE: %d | SPACING: %d/%d | WTD: %d | POS: %8d | READER: %8d | DATA: %5d", currentLine, (int) preSpacesCount, (int) spacesCount, wikiTagDepth, position, readerPos, writerPos);
        }
      }

      if (isWikiTag) {
        if (debug) {
          printf(" | WIKITAG | DATA: \"%s\" '%s'", readData, wikiTagNames[wikiTagType]);
        } else if (beVerbose) {
          printf("WIKITAG   | LINE: %8d | %8d | DATA: \"%s\" '%s'", currentLine, position, readData, wikiTagNames[wikiTagType]);
        }

        // TODO: Add wikiTag handling routine
        if (addWikiTag(0, xmlTag, formatType, formatGroup, wikiTagType, preSpacesCount, spacesCount, position, readData, dictFile, wtagFile, dictReadFile)) {
          isAdded = true;
          ++wikiTagCount;
        }

      } else if (isEntity) {
        if (debug) {
          printf(" |  ENTITY | DATA: \"%s\" ", entityBuffer);
        } else if (beVerbose) {
          printf("ENTITY    | LINE: %8d | %8d | DATA: \"%s\"", currentLine, position, entityBuffer);
        }

        // TODO: Add entity handling routine
        if (addEntity(0, xmlTag, formatType, formatGroup, preSpacesCount, spacesCount, position, entityBuffer)) {
          isAdded = true;
          ++entityCount;
        }

      } else if (writerPos != 0) {
        // Is regular word with read in data
        if (debug) {
          printf(" |   WORD  | DATA: \"%s\" ", readData);
        } else if (beVerbose) {
          printf("WORD      | LINE: %8d | %8d | DATA: \"%s\"", currentLine, position, readData);
        }

        // TODO: Think of "pre" and "/pre" as switch for nonformatting parts
        if (addWord(0, xmlTag, writerPos, formatType, formatGroup, preSpacesCount, spacesCount, position, readData, dictFile, dictReadFile)) {
          isAdded = true;
          ++wordCount;
        }
      } else if (debug) {
        // Having empty sequence here, something might be wrong!
        printf("[DEBUG] --- EMPTY SEQUENCE --->> LINE: %d | POS:%d | WTD: %d", currentLine, readerPos, wikiTagDepth);
      }

      //------------------------------------------------------------------------

      if (!isAdded && ((!isEntity && writerPos != 0) || (isEntity && entityWritePos != 0))) {
        ++failedElements;
      }

      if (debug || beVerbose)  {
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

    ++readerPos;
  }
  //printf("outside line: %d\n", currentLine);

  if (beVerbose || debug) {
    printf("--------------------------------------------------------------------\n");
  }

  return 1;
}

//------------------------------------------------------------------------------


bool addWikiTag(short elementType, void *element, const char formatType, const bool formatGroup, const short wikiTagType, const unsigned char preSpacesCount, const unsigned char spacesCount, const unsigned int position, const char *readData, FILE *dictFile, FILE *wtagFile, FILE *dictReadFile) {
  /*
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
  tag->wikiTagFileIndex = wikiTagCount;
  tag->pipedWords = NULL;
  tag->pipedTags = NULL;
  tag->pipedEntities = NULL;

  unsigned int dataLength = strlen(readData);
  char *pipePlace = strchr(readData, '|');
  unsigned int pipeStart = pipePlace != NULL ? pipePlace - readData: 0;

  if (pipeStart != 0) {
    if (debug)
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
          fprintf(wtagFile, "%d|%d|%s\n", wikiTagCount, writerPos - 1, parserData);

          byteWikiTags += (writerPos-1);

          if (debug)
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

              byteWhitespace += 2;

              ++readerPos;
              continue;
            case ' ':
              ++byteWhitespace;
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

              for (unsigned short i = 0; i < _formats; ++i) {
                if (strcmp(formats[i], formatData) == 0) {
                  if (dataFormatType == -1) {
                    dataFormatType = i;
                  } else if (dataFormatType == i) {
                    doCloseFormat = true;
                  } else {
                    // TODO: Add newst format handling code
                    dataHasParentFormat = true;
                  }

                  writerPos = 0;
                  break;
                }
              }

              break;
            case '|':
              ++byteWikiTags;
              if (!isWikiTag) {
                createWord = true;
              }
              break;
            case '[':
            case '{':
              tmpChar = readData[readerPos+1];
              if (tmpChar == readIn) {
                ++wikiTagDepth;

                if (!isWikiTag) {

                  formatReaderPos = readerPos;
                  formatDataPos = 0;

                  do {
                    tmpChar = readData[formatReaderPos];

                    if (formatDataPos == 64 || (formatReaderPos > 1 && tmpChar == ':') || tmpChar == '\0') {
                      break;
                    }

                    formatData[formatDataPos] = tolower(tmpChar);

                    ++formatDataPos;
                    ++formatReaderPos;
                  } while (tmpChar != ' ' && tmpChar != '|');

                  formatData[formatDataPos] = '\0';

                  for (unsigned short i = 0; i < _tagTypes; ++i) {
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
            if (debug) {
              printf("[DEBUG] FOUND [ ENTITY  ] => \"%s\"", entityBuffer);

              if (dataFormatType != -1) {
                printf(" => %s", formatNames[dataFormatType]);
              }
            }

            tag->pipedEntities = (entity*) realloc(tag->pipedEntities, sizeof(entity) * (tag->entityCount+1));
            if (addEntity(1, tag, formatType, formatGroup, preSpacesCount, spacesCount, position, entityBuffer)) {
              isAdded = true;
              ++entityCount;
            }

          } else if (isWikiTag) {
            if (debug) {
              printf("[DEBUG] FOUND [ WIKITAG ] => \"%s\" (%s)", parserData, wikiTagNames[tagType]);

              if (dataFormatType != -1) {
                printf(" => %s\n", formatNames[dataFormatType]);
              } else {
                printf("\n");
              }
            }

            tag->pipedTags = (wikiTag*) realloc(tag->pipedTags, sizeof(wikiTag) * (tag->wTagCount+1));
            if (addWikiTag(1, tag, formatType, formatGroup, tagType, preSpacesCount, spacesCount, position, parserData, dictFile, wtagFile, dictReadFile)) {
              isAdded = true;
              ++wikiTagCount;
            }

          } else {
            if (debug) {
              printf("[DEBUG] FOUND [ WORD    ] => \"%s\"", parserData);

              if (dataFormatType != -1) {
                printf(" => %s", formatNames[dataFormatType]);
              }
            }

            tag->pipedWords = (word*) realloc(tag->pipedWords, sizeof(word) * (tag->wordCount+1));
            if (addWord(1, tag, writerPos, formatType, formatGroup, preSpacesCount, spacesCount, position, parserData, dictFile, dictReadFile)) {
              isAdded = true;
              ++wordCount;
            }

          }

          if (!isAdded && ((!isEntity && writerPos != 0) || (isEntity && entityWritePos != 0))) {
            ++failedElements;
          }

          if (debug) {
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

    if (debug) {
      printf("\n");
    }

  } else {
    if (debug)
      printf("\n[DEBUG] ADDING WIKITAG  : \"%s\" (%s)", readData, wikiTagNames[wikiTagType]);

    //tag->target = malloc(sizeof(char) * (dataLength + 1));
    //strcpy(tag->target, readData);
    fprintf(wtagFile, "%d|%d|%s", wikiTagCount, dataLength, readData);

    byteWikiTags += dataLength;
  }

  if (elementType == 0) {
    ++xmlTag->wTagCount;
    xmlTag->lastAddedType = 1;
    xmlTag->lastAddedElement = &tag;
  } else {
    ++parentTag->wTagCount;
  }

  byteWikiTags += strlen(tagTypes[wikiTagType])+2;

  // 0 WORD, 1 WIKITAG, 2 ENTITY

  return 1;
}

//------------------------------------------------------------------------------


bool addEntity(short elementType, void *element, const char formatType, const bool formatGroup, const unsigned char preSpacesCount, const unsigned char spacesCount, const unsigned int position, const char entityBuffer[]) {
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

  for (unsigned short i = 0; i < _entities; ++i) {
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
      byteEntites += strlen(entities[i][0])+2;

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


bool addWord(const short elementType, void *element, const unsigned int dataLength, const char formatType, const bool formatGroup, const unsigned char preSpacesCount, const unsigned char spacesCount, const unsigned int position, const char *readData, FILE* dictFile, FILE* dictReadFile) {
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

  rewind(dictReadFile);

  bool isIncluded = false;
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


  if (!isIncluded) {
    ++fileLine;
    tagWord->wordFileStart = fileLine;
    fprintf(dictFile, "%u|%s\n", dataLength, readData);
    fflush(dictFile);
  }

  free(fileIndex);
  free(wordLength);
  //tagWord->data = malloc(sizeof(char) * (dataLength+1));
  //strcpy(tagWord->data, readData);

  byteWords += dataLength;

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

  if (debug) {
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
