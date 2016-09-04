# wicked
A wikipedia dump parser written in C.

## In short
wicket parses wikipedia xml dumps. And should function as a foundation for an compressor.

## Status
wicked is work in progress. Currently XML nodes are handled and read out structured, including keys and values, as well as the data contained in the XML nodes which can consist of words, wikitags and entities. Only some wikitags are supported at present.

Word data is written out to "data/words.txt" - wikitag link targets to "data/wikitags.txt". Wikitags become further processed so that included wikitags are handled as well as the data containing the parent as well as sub wikitags.

To save memory, the feature to read in the data into RAM is commented out, but can be easily reenabled if desired by readding the data and target attributes in the structs >word< and >wikiTag< and uncommet the strcpy's in addWord and addWikiTag. Please be aware that freeing this RAM is commented out too in >freeXMLCollection< and >freeXMLCollectionTag<.

##Origin of "enwik8_small"

"enwik8_small" is a cut out of the wikipedia dump "enwik8" file of the Hutter Prize. Since "enwik8" is 100 MB huge, I cant provide it directly, so for testing the "enwik8_small" has to be sufficient.

But any other wikipedia dump from https://dumps.wikimedia.org/ should be fine to use too.
