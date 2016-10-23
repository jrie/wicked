# -*- coding: utf-8 -*-
import re

entityPattern = re.compile(r"&[#0-9\w]*;")

formats = ["'''''", "'''", "''", "======", "=====", "====", "===", "=="]
tagTypes = ["{{", "{|", "[["]
tagClosings = ["}}", "|}", "]]"]
cleanupSequences = ["|", "'", "}", "]", "="]

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
    if (x != 0 and x % onePercent == 0):
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
        if (x != 0 and x % onePercent == 0):
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
            before = tagTypes[0]
            after = tagClosings[0]
        elif tagType == 1:
            before = tagTypes[1]
            after = tagClosings[1]
        else:
            before = tagTypes[2]
            after = tagClosings[2]

        if (srcLineData[line].find(before+tag+after) != -1):
            srcLineData[line] = srcLineData[line].replace(before+tag+after, "", 1)
        else:
            srcLineData[line] = srcLineData[line].replace(before+tag, "", 1)

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
        if (x != 0 and x % onePercent == 0):
            print(".", end="", flush=True)

        lineData = dataLine.rstrip("\n").split("|", 3)
        line = int(lineData[0]) - 1
        length = int(lineData[1])
        formatType = int(lineData[2])
        word = lineData[3]

        srcLineData[line] = srcLineData[line].replace(word, "", 1)
        x += 1;


#-------------------------------------------------------------------------------
print("\n\nCleaning leftover \"|\" pipe characters, formatting and wikitag closings from input...")
for index, line in enumerate(srcLineData):
    srcLineData[index] = line.replace("|", "");

    for item in cleanupSequences:
        srcLineData[index] = srcLineData[index].replace(item, "");


#-------------------------------------------------------------------------------
print("\n\nMerging whitespace...")
for index, line in enumerate(srcLineData):
    matches = re.findall(r"[ ]{2,}", line);
    if (matches != None):
        for match in matches:
            srcLineData[index] = srcLineData[index].replace(match, "");

#-------------------------------------------------------------------------------

print("\n\nWriting data to disk...")
with open("data/enwik8_small_shadow", "w") as srcFile:
    for line in srcLineData:
        srcFile.write(line)


#-------------------------------------------------------------------------------


print("\nDone. Bye bye.")
