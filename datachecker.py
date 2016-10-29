# -*- coding: utf-8 -*-
import re
from string import ascii_letters

entityPattern = re.compile(r"&[#0-9\w]*;")

formats = ["'''''", "'''", "''", "======", "=====", "====", "===", "=="]
tagOpenings = ["{{", "{|", "[["]
tagTypes = [
    "",
    "",
    "category:",
    "media:",
    "file:",
    "image:",
    "sound:",
    "wiktionary:",
    "wikipedia:",
    ""
]

tagClosings = ["}}", "|}", "]]"]
cleanupSequences = ["|", "}", "]"]

orphands = ["|", "{", "[", "'", "="]

srcLineData = [];

#-------------------------------------------------------------------------------

print("\nReading source data...")
with open("data/enwik8_small", "r") as srcFile:
    for line in srcFile:
        srcLineData.append(line);

#-------------------------------------------------------------------------------

print("\nRemoving entities...")
onePercent = round(len(srcLineData) / 100.0)
x = 0

for index, line in enumerate(srcLineData):
    if x != 0 and x % onePercent == 0:
        print(".", end="", flush=True)

    entities = re.findall(entityPattern, line)
    if entities != None:
        for entity in entities:
            srcLineData[index] = srcLineData[index].replace(entity, "", 1)

    x += 1

#-------------------------------------------------------------------------------

print("\n\nRemoving wiki tags...")
with open("data/wikitags.txt") as wikitagFile:
    x = 0
    for line in wikitagFile:
        x += 1

    wikitagFile.seek(0)

    onePercent = round(x / 100.0)
    x = 0

    for dataLine in wikitagFile:
        if x != 0 and x % onePercent == 0:
            print(".", end="", flush=True)

        lineData = dataLine.rstrip("\n").split("|", 4)
        line = int(lineData[0])-1
        tagType = int(lineData[1])
        formatType = int(lineData[2])
        length = int(lineData[3])
        tag = lineData[4]

        entities = re.findall(entityPattern, tag)
        if entities != None:
            for entity in entities:
                tag = tag.replace(entity, "", 1)

        if tagType == 0:
            before = tagOpenings[0]
            after = tagClosings[0]
        elif tagType == 1:
            before = tagOpenings[1]
            after = tagClosings[1]
        else:
            before = tagOpenings[2]
            after = tagClosings[2]

        if tagTypes[tagType] != "":
            tagVariants = [tagTypes[tagType].capitalize(), tagTypes[tagType]]

            if (srcLineData[line].find(before+tagVariants[0]+tag+after) != -1):
                srcLineData[line] = srcLineData[line].replace(before+tagVariants[0]+tag+after, "    ", 1)
            elif (srcLineData[line].find(before+tagVariants[1]+tag+after) != -1):
                srcLineData[line] = srcLineData[line].replace(before+tagVariants[1]+tag+after, "    ", 1)
            else:
                if (srcLineData[line].find(before+tagVariants[0]+tag) != -1):
                    srcLineData[line] = srcLineData[line].replace(before+tagVariants[0]+tag, "    ", 1)
                else:
                    srcLineData[line] = srcLineData[line].replace(before+tagVariants[1]+tag, "    ", 1)

                srcLineData[line] = srcLineData[line].replace(after, "")
        else:
            if (srcLineData[line].find(before+tag) != -1):
                srcLineData[line] = srcLineData[line].replace(before+tag, "    ", 1)
            else:
                srcLineData[line] = srcLineData[line].replace(before+tag, "    ", 1)

            srcLineData[line] = srcLineData[line].replace(after, "")

        if formatType != -1:
            formatting = formats[formatType]
            formatLength = len(formatting)

            pos = srcLineData[line].find(formatting)

            replacements = 0
            while pos != -1:
                previousChar = ""
                nextChar = ""

                if pos > 0:
                    previousChar = srcLineData[line][pos-1]

                if pos < len(srcLineData[line]):
                    nextChar = srcLineData[line][pos+formatLength]

                if formatType < 3:
                    if nextChar != "'" and previousChar != "'":
                        srcLineData[line] = srcLineData[line][:pos] + srcLineData[line][pos+len(formatting):]
                        replacements += 1
                        pos = srcLineData[line].find(formatting, pos)
                    else:
                        pos = srcLineData[line].find(formatting, pos+1)

                elif nextChar != "=" and previousChar != "=":
                    srcLineData[line] = srcLineData[line][:pos] + srcLineData[line][pos+len(formatting):]
                    replacements += 1
                    pos = srcLineData[line].find(formatting, pos)
                else:
                    pos = srcLineData[line].find(formatting, pos+1)

                if replacements == 2:
                    break





        x += 1;

#-------------------------------------------------------------------------------

print("\n\nRemoving words...")
with open("data/words.txt") as wordFile:

    x = 0
    for line in wordFile:
        x += 1

    wordFile.seek(0)

    onePercent = round(x / 100.0)
    x = 0

    for dataLine in wordFile:
        if x != 0 and x % onePercent == 0:
            print(".", end="", flush=True)

        lineData = dataLine.rstrip("\n").split("|", 3)
        line = int(lineData[0]) - 1
        length = int(lineData[1])
        formatType = int(lineData[2])
        word = lineData[3]

        if formatType != -1:
            formatting = formats[formatType]
            formatLength = len(formatting)

            pos = srcLineData[line].find(formatting)

            replacements = 0
            while pos != -1:
                previousChar = ""
                nextChar = ""

                if pos > 0:
                    previousChar = srcLineData[line][pos-1]

                if pos < len(srcLineData[line]):
                    nextChar = srcLineData[line][pos+formatLength]

                if formatType < 3:
                    if nextChar != "'" and previousChar != "'":
                        srcLineData[line] = srcLineData[line][:pos] + srcLineData[line][pos+len(formatting):]
                        replacements += 1
                        pos = srcLineData[line].find(formatting, pos)
                    else:
                        pos = srcLineData[line].find(formatting, pos+1)

                elif nextChar != "=" and previousChar != "=":
                    srcLineData[line] = srcLineData[line][:pos] + srcLineData[line][pos+len(formatting):]
                    replacements += 1
                    pos = srcLineData[line].find(formatting, pos)
                else:
                    pos = srcLineData[line].find(formatting, pos+1)

                if replacements == 2:
                    break

        srcLineData[line] = srcLineData[line].replace(word, "    ", 1)
        x += 1;


#-------------------------------------------------------------------------------

#print("\n\nCleaning leftover \"|\" pipe characters, formatting and wikitag closings from input...")
print("\n\nCleaning leftover \"|\" pipe characters...")
for index, line in enumerate(srcLineData):

    pos = srcLineData[index].find("|")

    while pos != -1:
        before = ""
        after = ""
        if pos > 0:
            before = srcLineData[index][pos-1]

        if pos < len(srcLineData[index]):
            after = srcLineData[index][pos+1]

        if (before in orphands or after in orphands) or (before == " " or after == " "):
            srcLineData[index] = srcLineData[index].replace("|", "", 1);

        pos = srcLineData[index].find("|", pos+1)

    """
    srcLineData[index] = line.replace("|", "");

    for item in cleanupSequences:
        srcLineData[index] = srcLineData[index].replace(item, "");
    """

#-------------------------------------------------------------------------------
print("\n\nMerging whitespace...")
for index, line in enumerate(srcLineData):
    matches = re.findall(r"[ ]{2,}", line);
    if (matches != None):
        for match in matches:
            srcLineData[index] = srcLineData[index].replace(match, "");

#-------------------------------------------------------------------------------

print("\n\nWriting shadow data to disk...")
with open("data/enwik8_small_shadow", "w") as srcFile:
    for line in srcLineData:
        srcFile.write(line.rstrip()+"\n")

#-------------------------------------------------------------------------------

print("\n\n=========================================================================\nReporting orphands\n=========================================================================\n")

for index, line in enumerate(srcLineData):
    line = line.lstrip()

    if (line == ""):
        continue;

    if line[0] == '<':
        posStart = line.find(">", 1)
        posEnd = line.rfind("<")

        if posEnd != -1 and posEnd > posStart:
            line = line[posStart+1:posEnd].rstrip()
        else:
            if posStart != -1:
                line = line[posStart+1:].rstrip()

    isReported = False
    for orphand in orphands:
        if orphand in line:
            print("Orphand symbols in line %d: %s\n" %(index+1, line.strip()))
            isReported = True
            break

    if not isReported:
        for item in ascii_letters:
            if item in line:
                print("Orphand strings in line %d: %s\n" %(index+1, line.strip()))
                break

#-------------------------------------------------------------------------------

print("\nDone. Bye bye.")
