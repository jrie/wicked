//------------------------------------------------------------------------------
// Author: Jan Rie <jan@dwrox.net>
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
#define DEBUG false
#define DEBUG_LARGE false
#define VERBOSE true
#define DOWRITEOUT true
#define LINESTOPROCESS 0

// -----------------------------------------------------------------------------
#define OUTPUTFILE "output.txt"
#define DICTIONARYFILE "words_sort.txt"
#define WIKITAGSFILE "wikitags_sort.txt"
#define XMLTAGFILE "xmltags_sort.txt"
#define XMLDATAFILE "xmldata_sort.txt"
#define ENTITIESFILE "entities_sort.txt"

// Buffers
#define XMLTAG_BUFFER 5120
#define IDENTATIONBUFFER 512
#define SPACESBUFFER 512

// Counts of predefined const datatypes
#define FORMATS 8
#define ENTITIES 211
#define INDENTS 3
#define TEMPLATES 11
#define TAGTYPES 11
#define TAGCLOSINGS 3
#define MATHTAG 0

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
  NOTE: Indents are only used at the beginning of the line!
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
  "Math type",
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

const char tagTypes[TAGTYPES][18] = {
  // NOTE: Handle definitions after, in case we threat, templates too
  "{{math", // https://en.wikipedia.org/wiki/Wikipedia:Rendering_math
  "{{", // Definition => {{Main|autistic savant}}
  "{\t", // Table start => ! (headline), |- (seperation, row/border), | "entry/ column data"
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
  {"#8SPACESBUFFER0", "\xe2\x82\x80"},
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

typedef struct xmldatakv {
  char *key;
  char *value;
} xmldatakv;

typedef struct entry {
  unsigned short elementType; // 0 XMLTAG, 1 WIKITAG, 2 WORD, 3 ENTITY
  unsigned int position;
  unsigned int start;
  unsigned int end;
  unsigned int preSpacesCount;
  unsigned int spacesCount;
  unsigned int length;
  unsigned int tagLength;
  short tagType;
  short dataFormatType;
  short ownFormatType;
  short hasPipe;
  short isHandledTag;
  short isDataNode;
  short isClosed;
  short xmlDataCount;
  int formatStart;
  int formatEnd;
  int connectedTag;
  struct xmldatakv *xmlData;
  char *stringData;
} entry;

typedef struct collection {
  FILE *outputFile;
  FILE *dictFile;
  FILE *wtagFile;
  FILE *xmltagFile;
  FILE *xmldataFile;
  FILE *entitiesFile;
  long int dictSize;
  long int wtagSize;
  long int xmltagSize;
  long int xmldataSize;
  long int entitiesSize;
  long int readDict;
  long int readwtag;
  long int readxmltag;
  long int readxmldata;
  long int readentities;
  unsigned int countWords;
  unsigned int countWtag;
  unsigned int countXmltag;
  unsigned int countEntities;
  struct entry *entriesWords;
  struct entry *entriesWtag;
  struct entry *entriesXmltag;
  struct entry *entriesEntities;
} collection;

enum dataTypes {
    XMLTAG = 0,
    WIKITAG = 1,
    WORD = 2,
    ENTITY = 3
};

//------------------------------------------------------------------------------
// Function declarations
bool readXMLtag(struct collection*);
bool readWord(struct collection*);
bool readEntity(struct collection*);
bool readWikitag(struct collection*);

bool freeEntryByLine(struct collection*, short, unsigned int, unsigned int);
struct entry* getEntryByType(struct collection*, short);
struct entry* getInLine(struct collection*, short, unsigned int, unsigned int, unsigned int*);

// ----------------------------------------------------------------------------

bool freeEntryByLine(struct collection *readerData, short elementType, unsigned int lineNum, unsigned int position) {
  unsigned int count = 0;
  struct entry* items = NULL;
  if (elementType == XMLTAG) {
    count = readerData->countXmltag;
    items = readerData->entriesXmltag;
  } else if (elementType == WORD) {
    count = readerData->countWords;
    items = readerData->entriesWords;
  } else if (elementType == WIKITAG) {
    count = readerData->countWtag;
    items = readerData->entriesWtag;
  } else {
    count = readerData->countEntities;
    items = readerData->entriesEntities;
  }

  if (count == 1) {
    if ((items[0].start == lineNum || items[0].end == lineNum) && items[0].position == position) {
      if (items[0].stringData) free(items[0].stringData);

      if (elementType == XMLTAG) {
        for (unsigned int j = 0; j < items[0].xmlDataCount; ++j) {
          if (items[0].xmlData[j].key) free(items[0].xmlData[j].key);
          if (items[0].xmlData[j].value) free(items[0].xmlData[j].value);
        }

        if (items[0].xmlData) free(items[0].xmlData);
      }

      if (elementType == XMLTAG) --readerData->countXmltag;
      else if (elementType == WORD) --readerData->countWords;
      else if (elementType == WIKITAG) --readerData->countWtag;
      else --readerData->countEntities;

      return true;
    }
  }

  for (unsigned int i = count - 1; i != 0; --i) {
    if ((items[i].start == lineNum || items[i].end == lineNum) && items[i].position == position) {
      if (i == count - 1) {
        if (items[i].stringData) free(items[i].stringData);

        if (elementType == XMLTAG) {
          for (unsigned int j = 0; j < items[i].xmlDataCount; ++j) {
            if (items[i].xmlData[j].key) free(items[i].xmlData[j].key);
            if (items[i].xmlData[j].value) free(items[i].xmlData[j].value);
          }

          if (items[i].xmlData) free(items[i].xmlData);
        }

        items = (struct entry *) realloc(items, sizeof(struct entry) * count);
        if (elementType == XMLTAG) --readerData->countXmltag;
        else if (elementType == WORD) --readerData->countWords;
        else if (elementType == WIKITAG) --readerData->countWtag;
        else --readerData->countEntities;

        return true;
      } else {
        if (items[i].stringData) free(items[i].stringData);

        if (elementType == XMLTAG) {
          for (unsigned int j = 0; j < items[i].xmlDataCount; ++j) {
            if (items[i].xmlData[j].key) free(items[i].xmlData[j].key);
            if (items[i].xmlData[j].value) free(items[i].xmlData[j].value);
          }

          if (items[i].xmlData) free(items[i].xmlData);
        }

        memmove(&items[i], &items[i + 1], sizeof(struct entry) * (count - i - 1));

        if (elementType == XMLTAG) --readerData->countXmltag;
        else if (elementType == WORD) --readerData->countWords;
        else if (elementType == WIKITAG) --readerData->countWtag;
        else --readerData->countEntities;

        return true;
      }
    }
  }

  return false;
}

//------------------------------------------------------------------------------

struct entry* getEntryByType(struct collection *readerData, short elementType) {
  if (elementType == XMLTAG) {
    if (readerData->countXmltag == 0) return NULL;
    return &readerData->entriesXmltag[readerData->countXmltag - 1];
  } else if (elementType == WORD) {
    if (readerData->countWords == 0) return NULL;
    return &readerData->entriesWords[readerData->countWords - 1];
  } else if (elementType == WIKITAG) {
    if (readerData->countWtag == 0) return NULL;
    return &readerData->entriesWtag[readerData->countWtag - 1];
  } else {
    if (readerData->countEntities == 0) return NULL;
    return &readerData->entriesEntities[readerData->countEntities - 1];
  }

  return NULL;
}

//------------------------------------------------------------------------------

struct entry* getInLine(struct collection *readerData, short elementType, unsigned int lineNum, unsigned int position, unsigned int* lastPos) {
  unsigned int count = 0;
  struct entry* items = NULL;
  if (elementType == XMLTAG) {
    count = readerData->countXmltag;
    items = readerData->entriesXmltag;
  } else if (elementType == WORD) {
    count = readerData->countWords;
    items = readerData->entriesWords;
  } else if (elementType == WIKITAG) {
    count = readerData->countWtag;
    items = readerData->entriesWtag;
  } else {
    count = readerData->countEntities;
    items = readerData->entriesEntities;
  }

  if (count == 0) return NULL;

  if (position != 0) {
    for (unsigned int i = *lastPos ; i < count; ++i) {
      if (items[i].start == lineNum && items[i].end == lineNum && items[i].position == position) return &items[i];
      ++lastPos;
    }
  } else {
    for (unsigned int i = 0 ; i < count; ++i) {
      if (items[i].start == lineNum || items[i].end == lineNum) return &items[i];
    }
  }

  return NULL;
}


//------------------------------------------------------------------------------
// Main routine
int main(int argc, char *argv[]) {
  printf("[ INFO ] Starting transpiling on file \"%s\".\n", OUTPUTFILE);

  FILE *outputFile = fopen(OUTPUTFILE, "w");
  FILE *dictFile = fopen(DICTIONARYFILE, "r");
  FILE *wtagFile = fopen(WIKITAGSFILE, "r");
  FILE *xmltagFile = fopen(XMLTAGFILE, "r");
  FILE *xmldataFile = fopen(XMLDATAFILE, "r");
  FILE *entitiesFile = fopen(ENTITIESFILE, "r");

  fseek(dictFile, 0, SEEK_END);
  fseek(wtagFile, 0, SEEK_END);
  fseek(xmltagFile, 0, SEEK_END);
  fseek(xmldataFile, 0, SEEK_END);
  fseek(entitiesFile, 0, SEEK_END);

  struct collection readerData = { outputFile,
    dictFile, wtagFile, xmltagFile, xmldataFile, entitiesFile,
    ftell(dictFile), ftell(wtagFile), ftell(xmltagFile), ftell(xmldataFile), ftell(entitiesFile),
    0, 0, 0, 0, 0,
    0, 0, 0, 0,
    NULL, NULL, NULL, NULL
  };

  rewind(dictFile);
  rewind(wtagFile);
  rewind(xmltagFile);
  rewind(xmldataFile);
  rewind(entitiesFile);

  time_t startTime = time(NULL);

  unsigned int lineNum = 1;
  unsigned int position = 1;
  unsigned int lastPos = 0;

  bool hasReplaced = false;
  char preSpaces[SPACESBUFFER];
  char subSpaces[SPACESBUFFER];
  char indentation[IDENTATIONBUFFER];
  int currentDepth = 0;
  int previousDepth = currentDepth;
  int lastWikiTypePos = -1;
  short lastWikiType = -1;

  struct entry* tmp = NULL;
  struct entry* preTmp = NULL;
  struct entry* closeTag = NULL;

  readXMLtag(&readerData);
  readWikitag(&readerData);
  readWord(&readerData);
  readEntity(&readerData);

  while (LINESTOPROCESS == 0 || lineNum <= LINESTOPROCESS) {
    hasReplaced = false;
    previousDepth = currentDepth;

    #if VERBOSE
    printf("LINE NUM: %d [%x] - %d\n", lineNum, lineNum, position);
    #endif
    #if DEBUG_LARGE
    for (int i = 0, j = readerData.countXmltag; i < j; ++i) {
      struct entry* item = &readerData.entriesXmltag[i];
      printf("%d ::::: %d/%u --- %u - pipe: %d ===> %s\n", item->elementType, item->start, item->end, item->position, item->hasPipe, item->stringData);
    }
    for (int i = 0, j = readerData.countWtag; i < j; ++i) {
      struct entry* item = &readerData.entriesWtag[i];
      printf("%d ::::: %d/%u --- %u - pipe: %d ===> %s\n", item->elementType, item->start, item->end, item->position, item->hasPipe, item->stringData);
    }
    for (int i = 0, j = readerData.countWords; i < j; ++i) {
      struct entry* item = &readerData.entriesWords[i];
      printf("%d ::::: %d/%u --- %u - pipe: %d ===> %s\n", item->elementType, item->start, item->end, item->position, item->hasPipe, item->stringData);
    }
    for (int i = 0, j = readerData.countEntities; i < j; ++i) {
      struct entry* item = &readerData.entriesEntities[i];
      printf("%d ::::: %d/%u --- %u - pipe: %d ===> %s\n", item->elementType, item->start, item->end, item->position, item->hasPipe, item->stringData);
    }
    #endif

    memset(indentation, ' ', IDENTATIONBUFFER);
    indentation[currentDepth * 2] = '\0';

    lastPos = 0;
    if ((tmp = getInLine(&readerData, XMLTAG, lineNum, 0, &lastPos))) {
      if (tmp->start == lineNum && !tmp->isHandledTag) {
        tmp->isHandledTag = true;

        #if DEBUG
        printf("%s<%s", indentation, tmp->stringData);
        #endif

        #if DOWRITEOUT
        fprintf(outputFile, "%s<%s", indentation, tmp->stringData);
        #endif

        for (int j = 0; j < tmp->xmlDataCount; ++j) {
          #if DEBUG
          printf(" %s=\"%s\"", tmp->xmlData[j].key, tmp->xmlData[j].value);
          #endif

          #if DOWRITEOUT
          fprintf(outputFile, " %s=\"%s\"", tmp->xmlData[j].key, tmp->xmlData[j].value);
          #endif
        }

        if (!tmp->isDataNode) {
          if (tmp->start != tmp->end) {
            #if DEBUG
            printf("%s", ">\n");
            #endif

            #if DOWRITEOUT
            fprintf(outputFile, ">\n");
            #endif
          }
        } else {
          #if DEBUG
          printf("%s", ">");
          #endif

          #if DOWRITEOUT
          fputc('>', outputFile);
          #endif

          if (tmp->end == lineNum) {
            hasReplaced = true;
          }
        }

        if (tmp->start == tmp->end) closeTag = tmp;
        else {
          ++currentDepth;
          memset(indentation, ' ', IDENTATIONBUFFER);
          indentation[currentDepth * 2] = '\0';
        }
      } else if (tmp->end == lineNum) {
        closeTag = tmp;
      }
    }

    readXMLtag(&readerData);

    while(readWikitag(&readerData)) {
      if ((preTmp = getEntryByType(&readerData, WIKITAG)) && preTmp->start != lineNum) break;
    }

    lastPos = 0;
    while ((tmp = getInLine(&readerData, WIKITAG, lineNum, position, &lastPos))) {
      memset(preSpaces, ' ', tmp->preSpacesCount);
      memset(subSpaces, ' ', tmp->spacesCount);
      preSpaces[tmp->preSpacesCount] = '\0';
      subSpaces[tmp->spacesCount] = '\0';

      lastWikiTypePos = tmp->position;
      lastWikiType = tmp->tagType;

      #if DEBUG
      printf("%s%s%s", preSpaces, tagTypes[tmp->tagType], tmp->stringData);
      #endif

      #if DOWRITEOUT
      fprintf(outputFile, "%s%s%s", preSpaces, tagTypes[tmp->tagType], tmp->stringData);
      #endif

      if (tmp->hasPipe) {
        #if DEBUG
        printf("%s", "|");
        #endif

        #if DOWRITEOUT
        fputc('|', outputFile);
        #endif

      }

      freeEntryByLine(&readerData, WIKITAG, lineNum, position);
      hasReplaced = true;
      ++position;
    }


    while(readWord(&readerData)) {
      if ((preTmp = getEntryByType(&readerData, WORD)) && preTmp->start != lineNum) break;
    }

    lastPos = 0;
    while ((tmp = getInLine(&readerData, WORD, lineNum, position, &lastPos)))  {
      memset(preSpaces, ' ', tmp->preSpacesCount);
      memset(subSpaces, ' ', tmp->spacesCount);
      preSpaces[tmp->preSpacesCount] = '\0';
      subSpaces[tmp->spacesCount] = '\0';

      if (tmp->connectedTag != -1) {
        lastWikiTypePos = tmp->position;
        #if DEBUG
        printf("%s%s%s", preSpaces, tmp->stringData, subSpaces);
        #endif

        #if DOWRITEOUT
        fprintf(outputFile, "%s%s%s", preSpaces, tmp->stringData, subSpaces);
        #endif

        if (tmp->hasPipe) {
          #if DEBUG
          printf("%s", "|");
          #endif

          #if DOWRITEOUT
          fputc('|', outputFile);
          #endif
        }
      } else {
        #if DEBUG
        printf("%s%s%s", preSpaces, tmp->stringData, subSpaces);
        #endif

        #if DOWRITEOUT
        fprintf(outputFile, "%s%s%s", preSpaces, tmp->stringData, subSpaces);
        #endif
      }

      freeEntryByLine(&readerData, WORD, lineNum, position);
      hasReplaced = true;
      ++position;
    }

    while(readEntity(&readerData)) {
      if ((preTmp = getEntryByType(&readerData, ENTITY)) && preTmp->start != lineNum) break;
    }

    lastPos = 0;
    if ((tmp = getInLine(&readerData, ENTITY, lineNum, position, &lastPos))) {
      memset(preSpaces, ' ', tmp->preSpacesCount);
      memset(subSpaces, ' ', tmp->spacesCount);
      preSpaces[tmp->preSpacesCount] = '\0';
      subSpaces[tmp->spacesCount] = '\0';

      if (tmp->connectedTag != -1) {
        lastWikiTypePos = tmp->position;
        #if DEBUG
        printf("%s%s%s", preSpaces, tmp->stringData, subSpaces);
        #endif

        #if DOWRITEOUT
        fprintf(outputFile, "%s%s%s", preSpaces, tmp->stringData, subSpaces);
        #endif

        if (tmp->hasPipe) {
          #if DEBUG
          printf("%s", "|");
          #endif

          #if DOWRITEOUT
          fputc('|', outputFile);
          #endif
        }
      } else {
        #if DEBUG
        printf("%s%s%s", preSpaces, tmp->stringData, subSpaces);
        #endif
        #if DOWRITEOUT
        fprintf(outputFile, "%s%s%s", preSpaces, tmp->stringData, subSpaces);
        #endif
      }

      freeEntryByLine(&readerData, ENTITY, lineNum, position);
      hasReplaced = true;
      ++position;
    }

    if (lastWikiType != -1 && lastWikiTypePos != -1) {
      lastPos = 0;
      tmp = getInLine(&readerData, WORD, lineNum, position + 1, &lastPos);
      if (!tmp || (tmp && tmp->connectedTag == -1)) {
        // TODO: Add table closing here.
        char closing[3];
        strcpy(closing, lastWikiType > 3 ? tagClosingsTypes[1] : tagClosingsTypes[0]);
        #if DEBUG
        printf("%s", closing);
        #endif
        #if DOWRITEOUT
        fprintf(outputFile, "%s", closing);
        #endif

        lastWikiType = -1;
      }
    }

    if (hasReplaced) continue;

    lastPos = 0;
    if ((closeTag = getInLine(&readerData, XMLTAG, lineNum, 0, &lastPos))) {
      if (closeTag != NULL) {
        if (closeTag->isDataNode && closeTag->end == lineNum) {
          #if DEBUG
          printf("</%s>\n", closeTag->stringData);
          #endif

          #if DOWRITEOUT
          fprintf(outputFile, "</%s>\n", closeTag->stringData);
          #endif

          freeEntryByLine(&readerData, XMLTAG, lineNum, 0);
        } else if (closeTag->end == closeTag->start) {
          #if DEBUG
          printf(" />\n");
          #endif

          #if DOWRITEOUT
          fprintf(outputFile, " />\n");
          #endif

          freeEntryByLine(&readerData, XMLTAG, lineNum, 0);
        } else if (!closeTag->isDataNode && closeTag->end == lineNum) {
          if (currentDepth != 0) --currentDepth;
          memset(indentation, ' ', IDENTATIONBUFFER);
          indentation[currentDepth * 2] = '\0';

          #if DEBUG
          printf( "%s</%s>\n", indentation, closeTag->stringData);
          #endif

          #if DOWRITEOUT
          fprintf(outputFile, "%s</%s>\n", indentation, closeTag->stringData);
          #endif
          freeEntryByLine(&readerData, XMLTAG, lineNum, 0);
        } else if (closeTag->start != closeTag->end && closeTag->isDataNode) {
          #if DEBUG
          printf("%s", "\n");
          #endif
          #if DOWRITEOUT
          fputc('\n', outputFile);
          #endif
        }

        closeTag = NULL;
        readXMLtag(&readerData);
      }
    } else {
      #if DEBUG
      printf("%s", "\n");
      #endif

      #if DOWRITEOUT
      fputc('\n', outputFile);
      #endif

      if (currentDepth > 0 && currentDepth == previousDepth) --currentDepth;
    }

    position = 1;
    ++lineNum;
  }


  long int duration = difftime(time(NULL), startTime);
  long int durHours = floor(duration / 3600);
  long int durMinutes = floor((duration % 3600) / 60);
  long int durSeconds = (duration % 3600) % 60;
  printf("\n\n[STATUS] TRANSPILLING DATA PROCESS: %ldh %ldm %lds\n", durHours, durMinutes, durSeconds);
  // Cleanup
  fclose(readerData.outputFile);
  fclose(readerData.dictFile);
  fclose(readerData.wtagFile);
  fclose(readerData.xmltagFile);
  fclose(readerData.xmldataFile);
  fclose(readerData.entitiesFile);

  for (unsigned int i = 0; i < readerData.countXmltag; ++i) {
    if (readerData.entriesXmltag[i].stringData) free(readerData.entriesXmltag[i].stringData);

    for (unsigned int j = 0; j < readerData.entriesXmltag[i].xmlDataCount; ++j) {
      if (readerData.entriesXmltag[i].xmlData[j].key) free(readerData.entriesXmltag[i].xmlData[j].key);
      if (readerData.entriesXmltag[i].xmlData[j].value) free(readerData.entriesXmltag[i].xmlData[j].value);
    }

    if (readerData.entriesXmltag[i].xmlData) free(readerData.entriesXmltag[i].xmlData);
  }

  for (unsigned int i = 0; i < readerData.countWords; ++i) {
    if (readerData.entriesWords[i].stringData) free(readerData.entriesWords[i].stringData);
  }

  for (unsigned int i = 0; i < readerData.countWtag; ++i) {
    if (readerData.entriesWtag[i].stringData) free(readerData.entriesWtag[i].stringData);
  }

  for (unsigned int i = 0; i < readerData.countEntities; ++i) {
    if (readerData.entriesEntities[i].stringData) free(readerData.entriesEntities[i].stringData);
  }

  free(readerData.entriesXmltag);
  free(readerData.entriesWords);
  free(readerData.entriesWtag);
  free(readerData.entriesEntities);
  return 0;
}

//------------------------------------------------------------------------------

bool readXMLtag(struct collection *readerData) {
  if (readerData->readxmltag == readerData->xmltagSize) return false;

  if ((readerData->entriesXmltag = (struct entry *) realloc(readerData->entriesXmltag, sizeof(struct entry) * (readerData->countXmltag + 1))) == NULL) {
    return false;
  }

  struct entry *element = &readerData->entriesXmltag[readerData->countXmltag];
  element->elementType = XMLTAG;
  element->stringData = NULL;
  element->start = 0;
  element->end = 0;
  element->isClosed = false;
  element->isDataNode = false;
  element->xmlData = NULL;
  element->xmlDataCount = 0;
  element->position = 1;
  element->connectedTag = -1;
  element->isHandledTag = false;
  element->hasPipe = false;

  fscanf(readerData->xmltagFile, "%x\t%x\t%hi\t%hi\t", &element->start, &element->end, &element->isClosed, &element->isDataNode);

  element->stringData = malloc(sizeof(char) * XMLTAG_BUFFER);

  unsigned int dataCount = 0;
  unsigned int start = 0;
  unsigned int end = 0;
  char tmpKey[256] = "\0";
  char tmpValue[1280] = "\0";

  if (readerData->readxmldata < readerData->xmldataSize) {
    unsigned int readBytes = 0;
    while (true) {
      readBytes = ftell(readerData->xmldataFile);
      if (readBytes == readerData->xmldataSize) break;
      fscanf(readerData->xmldataFile, "%x\t%x\t", &start, &end);

      if (start == element->start && end == element->end) {
        fscanf(readerData->xmldataFile, "%s\t%[^\n]s\n", tmpKey, tmpValue);
        element->xmlData = (struct xmldatakv *)realloc(element->xmlData, sizeof(struct xmldatakv) * (dataCount + 1));

        element->xmlData[dataCount].key = malloc(sizeof(char) * (strlen(tmpKey) + 1));
        strcpy(element->xmlData[dataCount].key, tmpKey);

        element->xmlData[dataCount].value = malloc(sizeof(char) * (strlen(tmpValue) + 1));
        strcpy(element->xmlData[dataCount].value, tmpValue);
        ++dataCount;

        #if DEBUG
        //printf("xmlData =>\t%s\t==>\t%s\n", tmpKey, tmpValue);
        #endif
      } else {
        fseek(readerData->xmldataFile, readBytes, SEEK_SET);
        break;
      }
    }

    element->xmlDataCount = dataCount;
    readerData->readxmldata = ftell(readerData->xmldataFile);

  }

  fscanf(readerData->xmltagFile, "%[^\n]s\n", element->stringData);

  readerData->readxmltag = ftell(readerData->xmltagFile);
  ++readerData->countXmltag;
  return true;
}

//------------------------------------------------------------------------------

bool readWikitag(struct collection *readerData) {
  if (readerData->readwtag == readerData->wtagSize) return false;

  if ((readerData->entriesWtag = (struct entry*) realloc(readerData->entriesWtag, sizeof(struct entry) * (readerData->countWtag + 1))) == NULL) {
    return false;
  }

  struct entry *element = &readerData->entriesWtag[readerData->countWtag];
  element->elementType = WIKITAG;
  element->stringData = NULL;
  element->xmlData = NULL;
  element->xmlDataCount = 0;
  element->start = 0;
  element->end = 0;
  element->preSpacesCount = 0;
  element->spacesCount = 0;
  element->position = 0;
  element->connectedTag = -1;
  element->tagType = 0;
  element->tagLength = 0;
  element->length = 0;
  element->dataFormatType = -1;
  element->ownFormatType = -1;
  element->formatStart = 0;
  element->formatEnd = 0;
  element->isHandledTag = false;
  element->hasPipe = false;

  fscanf(readerData->wtagFile, "%x\t%x\t%x\t%x\t%hi\t%hi\t%hi\t%d\t%d\t%d\t%d\t%hi\t", &element->position, &element->start, &element->preSpacesCount, &element->spacesCount, &element->tagType, &element->dataFormatType, &element->ownFormatType, &element->formatStart, &element->formatEnd, &element->tagLength, &element->length, &element->hasPipe);
  element->end = element->start;
  element->stringData = malloc(sizeof(char) * (element->length + 1));

  if (element->length == 0) {
    element->stringData[0] = '\0';
    fgetc(readerData->wtagFile);
    fgetc(readerData->wtagFile);
  } else if (element->length == 1) {
    element->stringData[0] = fgetc(readerData->wtagFile);
    element->stringData[1] = '\0';
    fgetc(readerData->wtagFile);
  } else {
    fscanf(readerData->wtagFile, "%[^\n]s\n", element->stringData);
  }
  readerData->readwtag = ftell(readerData->wtagFile);
  ++readerData->countWtag;
  return true;
}

//------------------------------------------ ------------------------------------

bool readWord(struct collection *readerData) {
  //printf("in word tag\n\n");
  if (readerData->readDict == readerData->dictSize) return false;

  if ((readerData->entriesWords = (struct entry*) realloc(readerData->entriesWords, sizeof(struct entry) * (readerData->countWords + 1))) == NULL) {
    return false;
  }

  struct entry *element = &readerData->entriesWords[readerData->countWords];
  element->elementType = WORD;
  element->stringData = NULL;
  element->xmlData = NULL;
  element->xmlDataCount = 0;
  element->start = 0;
  element->end = 0;
  element->preSpacesCount = 0;
  element->spacesCount = 0;
  element->position = 0;
  element->connectedTag = -1;
  element->tagType = 0;
  element->tagLength = 0;
  element->length = 0;
  element->dataFormatType = -1;
  element->ownFormatType = -1;
  element->formatStart = 0;
  element->formatEnd = 0;
  element->isHandledTag = false;
  element->hasPipe = false;

  fscanf(readerData->dictFile, "%x\t%x\t%d\t%x\t%x\t%u\t%hi\t%hi\t%d\t%d\t%hi\t", &element->position, &element->start, &element->connectedTag, &element->preSpacesCount, &element->spacesCount, &element->length, &element->dataFormatType, &element->ownFormatType, &element->formatStart, &element->formatEnd, &element->hasPipe);

  element->end = element->start;
  element->stringData = malloc(sizeof(char) * (element->length + 1));

  if (element->length == 0) {
    element->stringData[0] = '\0';
    fgetc(readerData->dictFile);
    fgetc(readerData->dictFile);
  } else if (element->length == 1) {
    element->stringData[0] = fgetc(readerData->dictFile);
    element->stringData[1] = '\0';
    fgetc(readerData->dictFile);
  } else {
    fscanf(readerData->dictFile, "%[^\n]s\n", element->stringData);
  }

  readerData->readDict = ftell(readerData->dictFile);
  ++readerData->countWords;
  return true;
}

//------------------------------------------------------------------------------

bool readEntity(struct collection *readerData) {
  //printf("in entity tag\n\n");
  if (&readerData->readentities == &readerData->entitiesSize) return false;

  if ((readerData->entriesEntities = (struct entry*) realloc(readerData->entriesEntities, sizeof(struct entry) * (readerData->countEntities + 1))) == NULL) {
    return false;
  }

  struct entry *element = &readerData->entriesEntities[readerData->countEntities];
  element->elementType = ENTITY;
  element->stringData = NULL;
  element->xmlData = NULL;
  element->xmlDataCount = 0;
  element->start = 0;
  element->end = 0;
  element->position = 0;
  element->connectedTag = -1;
  element->isHandledTag = false;
  element->hasPipe = false;

  fscanf(readerData->entitiesFile, "%x\t%x\t%d\t%x\t%x\t%hi\t%hi\t%d\t%d\t%hi\t", &element->position, &element->start, &element->connectedTag, &element->preSpacesCount, &element->spacesCount, &element->dataFormatType, &element->ownFormatType, &element->formatStart, &element->formatEnd, &element->hasPipe);
  element->end = element->start;
  element->stringData = malloc(sizeof(char) * 24);

  fscanf(readerData->entitiesFile, "%[^\n]s\n", element->stringData);

  readerData->readentities = ftell(readerData->entitiesFile);
  ++readerData->countEntities;
  return true;
}

//------------------------------------------------------------------------------