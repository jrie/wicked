# wicked
A wikipedia dump parser written in pure C.

## In short
*wicked* parses (wikipedia) xml dumps. And should function as a foundation for an compressor, reading out data, attributes, key and values of XML and Wikipedia tag data into RAM, but also saving out parsing results to files and delivering debug output to aid checking for errors or providing a file count/byte detection statistic.

## Status and further information
**wicked is work in progress.**

XML nodes are handled and read out structured into memory, including keys and values, as well as the data contained in the XML nodes which can consist of words, wikitags and entities.

By current defaults, wicked creates a lot of debug output which shows an outline of what data has been added, wikitag information styling as well as link targets, anchors, images. Multi-line tables are not supported yet, but this might be a feature to come in a further release.

Words data is written out to **words.txt** - wikitag link targets to **wikitags.txt**. Wikitags become further processed so that included words are handled as well as styling tags. Entities are written to **entities.txt**.

Each of this elements contains background information about pre and postscpacing, styling information, position in the row by index, if its a format start or end and other details inside *wicked*. I would recommend checking out the data *struct word*, *struct wikitag* and *struct entity* as well as the others.

#### datachecker.py

**datachecker.py** is a small written in Python3 to filter out collected information stored in the **.txt** element files. It does consume a lot of memory and heavily uses regular expressions. It reports all left over information from *wikipedia dump* to **orphandReport.txt** which consists of all lines containing some kind of data, which has not been succesfully replaced by all other clearing routines of *datachecker.py*.

## Origin of "enwik8_small"

"enwik8_small" is a cut out of the wikipedia dump "enwik8" file of the **Hutter Prize**. Since *enwik8* is at 100 MB in size, I dont provide it directly here on github, so for testing the "enwik8_small" has to be sufficient. But I would recommend using "enwik8" or a current wikipedia (meta) dump or similar for testing *wicked*. But any other wikipedia dump from https://dumps.wikimedia.org/ should be fine to use too.
