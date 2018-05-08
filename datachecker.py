#!/usr/bin/python3
# -*- coding: utf-8 -*-
from re import escape, compile, search, sub, finditer, purge
from sys import stdout
from string import ascii_letters
from math import floor

# Constants
LINESTOPROCESS = 5000
LINESTOPROCESS = 1085
LINESTOPROCESS = 238
LINESTOPROCESS = 0
PRINTWIKI = False
PRINTWORD = False
PRINTENTITY = False
PRINTWIKI = True
#PRINTENTITY = True
#PRINTWORD = True
DOSAVE = True

FINDTOLERANCE = 2

#-------------------------------------------------------------------------------
#srcFilePath = "data/enwiki-20160720-pages-meta-current1.xml-p000000010p000030303"
srcFilePath = "data/enwik8_small"
#srcFilePath = "data/enwik8"
wikiTagPath = 'wikitags.txt'
xmlTagPath = 'xmltags.txt'
wordPath = 'words.txt'
entityPath = 'entities.txt'
"""
wikiTagPath = 'data/wikitags.txt'
xmlTagPath = 'data/xmltags.txt'
wordPath = 'data/words.txt'
entityPath = 'data/entities.txt'
"""

#-------------------------------------------------------------------------------
# Internals
formats = ["'''''", "'''", "''", '======', '=====', '====', '===', '==']
formatsReal = ["'''''", "'''", "''", '======', '=====', '====', '===', '==']
tagOpenings = ['{{', '{|', '[[']
tagOpeningsReal = ['{{', '{|', '[[']
tagClosings = ['}}', '|}',  ']]']
tagClosingsReal = ['}}', '|}',  ']]']
#cleanupSequences = ['|', '}', ']', '{', '[']
cleanupSequences = ['|']
tagTypes = [
    '',
    '',
    'category:',
    'media:',
    'file:',
    'image:',
    'sound:',
    'wiktionary:',
    'wikipedia:',
    ''
]

for index, value in enumerate(formats):
    formats[index] = escape(value)

for index, value in enumerate(tagOpenings):
    tagOpenings[index] = escape(value)

for index, value in enumerate(tagClosings):
    tagClosings[index] = escape(value)

for index, value in enumerate(tagTypes):
    tagTypes[index] = escape(value)

srcLineData = []
outputData = []
outputDataLineNums = []

# Start 1

print('\nReading source data...')
with open(srcFilePath, 'r') as srcFile:
    for index, line in enumerate(srcFile):
        if LINESTOPROCESS != 0 and index == LINESTOPROCESS:
            break
        srcLineData.append(line);

#-------------------------------------------------------------------------------
def removeWikiTags(srcLineData):
    print('\n\nRemoving wiki tags...')

    divider = '#' * 30
    divider3 = '-' * 15

    preRegEx1 = []
    preRegEx2 = []

    with open(wikiTagPath, 'r') as wikitagFile:
        x = 0
        for x, line in enumerate(wikitagFile):
            continue

        onePercent = floor(x / 100.0)
        if onePercent == 0: onePercent = 1

        wikitagFile.seek(0)

        for x, dataLine in enumerate(wikitagFile):
            if x % onePercent == 0:
                stdout.write('.')
                stdout.flush()

            lineData = dataLine.rstrip('\n').split('|', 12)
            position = int(lineData[0])
            lineNum = int(lineData[1]) - 1
            readerPos = int(lineData[2]) - 1
            preSpacesCount = int(lineData[3])
            spacesCount = int(lineData[4])
            tagType = int(lineData[5])
            formatType = int(lineData[6])
            ownFormat = int(lineData[7])
            isFormatStart = int(lineData[8])
            isFormatEnd = int(lineData[9])
            tagLength = int(lineData[10])
            length = int(lineData[11])
            tag = escape(lineData[12])

            if LINESTOPROCESS != 0 and lineNum > LINESTOPROCESS:
                purge()
                return srcLineData

            preRegEx1 = []
            preRegEx2 = []

            try:
                workLine = srcLineData[lineNum]
            except:
                continue

            lengthEx1 = length;
            lengthEx2 = 0;

            if isFormatStart:
                if ownFormat != -1:
                    preRegEx1.append(formats[ownFormat])
                    lengthEx1 += len(formatsReal[ownFormat])
                else:
                     preRegEx1.append(formats[formatType])
                     lengthEx1 += len(formatsReal[formatType])

            if preSpacesCount != 0:
                preRegEx1.append('[\s]{'+str(preSpacesCount)+'}')
                lengthEx1 += preSpacesCount

            if tagType == 0:
                preRegEx1.append(tagOpenings[0])
                preRegEx2.append(tagClosings[0])
                lengthEx1 += len(tagOpeningsReal[0])
                lengthEx2 += len(tagClosingsReal[0])
            elif tagType == 1:
                preRegEx1.append(tagOpenings[1])
                preRegEx2.append(tagClosings[1])
                lengthEx1 += len(tagOpeningsReal[1])
                lengthEx2 += len(tagClosingsReal[1])
            else:
                preRegEx1.append(tagOpenings[2])
                preRegEx2.append(tagClosings[2])
                lengthEx1 += len(tagOpeningsReal[2])
                lengthEx2 += len(tagClosingsReal[2])


            preRegEx1.append(tag)

            if spacesCount != 0:
                preRegEx2.append('[\s]{'+str(spacesCount)+'}')
                lengthEx2 += spacesCount

            if isFormatEnd:
                if ownFormat != -1:
                    preRegEx2.append(formats[ownFormat])
                    lengthEx2 += len(formatsReal[ownFormat])
                else:
                    preRegEx2.append(formats[formatType])
                    lengthEx2 += len(formatsReal[formatType])

            regExString = compile(''.join(preRegEx1))
            regExString2 = compile(''.join(preRegEx2))

            if LINESTOPROCESS != 0 and lineNum + 1 == LINESTOPROCESS and PRINTWIKI:
                print()
                print(divider)
                print(lineNum, lineData)
                print(regExString.pattern)
                print(regExString2.pattern)
                print(divider3)
                print(workLine)

            hasReplaced = False
            for match in finditer(regExString,workLine):
                workLine = regExString.sub(' ' * lengthEx1, workLine, 1)
                hasReplaced = True
                break

            if not hasReplaced:
                if preSpacesCount != 0:
                    needle = r'[\s]{'+str(preSpacesCount)+r'}'

                    for index, item in enumerate(preRegEx1):
                        if item == needle:
                            del preRegEx1[index]
                            preRegEx1.insert(index-1, needle)
                            break

                if spacesCount != 0:
                    needle = r'[\s]{'+str(spacesCount)+r'}'

                    for index, item in enumerate(preRegEx2):
                        if item == needle:
                            del preRegEx2[index]
                            preRegEx2.insert(index+1, needle)
                            break

                regExString = compile(''.join(preRegEx1)+''.join(preRegEx2))

                for match in finditer(regExString, workLine):
                    workLine = regExString.sub(' ' * lengthEx1, workLine, 1)
                    break

            for match in finditer(regExString2, workLine):
                findSpot = match.span()
                if lineNum + 1 == LINESTOPROCESS:
                    print(lineData)
                    print(findSpot)
                    exit()
                if readerPos + tagLength <= findSpot[0] :
                    srcLineData[lineNum] = workLine[:findSpot[0]] + (' ' * lengthEx2)+ workLine[findSpot[1]:]
                    #srcLineData[lineNum] = regExString2.sub(' ' * lengthEx2, workLine, 1)
                    break


            if LINESTOPROCESS != 0 and lineNum + 1 == LINESTOPROCESS and PRINTWIKI:
                print(divider3)
                print(srcLineData[lineNum])

    purge()
    return srcLineData

#--------------------------------------------------------------------------------------------------------------------------
def removeXMLTags(srcLineData):
    print('\n\nCleaning xml tags...')

    preRegEx1 = []
    preRegEx2 = []

    with open(xmlTagPath, 'r') as xmlFile:
        x = 0
        for line in xmlFile:
            x += 1

        onePercent = floor(x / 100.0)
        if onePercent == 0: onePercent = 1
        x = 0

        xmlFile.seek(0)

        for dataLine in xmlFile:
            if x % onePercent == 0:
                stdout.write('.')
                stdout.flush()

            lineData = dataLine.rstrip('\n').split('|', 4)
            start = int(lineData[0]) - 1
            end = int(lineData[1]) - 1
            isClosed = int(lineData[2])
            isDataNode = int(lineData[3])
            tagName = escape(lineData[4])

            if LINESTOPROCESS != 0 and start > LINESTOPROCESS:
                purge()
                return srcLineData

            workLine = srcLineData[start]
            regExString = compile(r'<'+tagName+r'[^>]{0,}>')
            srcLineData[start] = sub(regExString, ' ', workLine, 1).lstrip()

            if LINESTOPROCESS != 0:
                if end <= LINESTOPROCESS:
                    regExString = compile(r'<[\/]{0,1}'+tagName+r'[^>\/]{0,}>')
                    srcLineData[end] = sub(regExString, ' ', srcLineData[end], 1)
            else:
                regExString = compile(r'<[\/]{0,1}'+tagName+r'[^>\/]{0,}>')
                srcLineData[end] = sub(regExString, ' ', srcLineData[end], 1)

            x += 1

    purge()
    return srcLineData

#-------------------------------------------------------------------------------
def removeWords(srcLineData):

    print('\n\nRemoving words...')
    sortedWordList = {}
    x = 0

    with open(wordPath, 'r') as wordFile:
        for x, line in enumerate(wordFile):
            lineData = line.rstrip('\n').split('|', 11)
            position = int(lineData[0])
            lineNum = int(lineData[1]) - 1

            lineData = lineData[2:]

            try:
                sortedWordList[lineNum]

                for index, entry in enumerate(lineData):
                    if index < 9:
                        lineData[index] = int(entry)
                    else:
                        lineData[index] = entry

                sortedWordList[lineNum].append(lineData)
            except:
                sortedWordList[lineNum] = []

                for index, entry in enumerate(lineData):
                    if index < 9:
                        lineData[index] = int(entry)
                    else:
                        lineData[index] = entry

                sortedWordList[lineNum].append(lineData)

    onePercent = floor(x / 100.0)
    if onePercent == 0: onePercent = 1
    x = 0

    divider = '#' * 90
    divider3 = '-' * 45
    preRegEx1 = []
    preRegEx2 = []

    borderline = LINESTOPROCESS - 1


    for lineNum in sortedWordList.keys():
        try:
            workLine = srcLineData[lineNum]
        except:
            continue

        for lineData in sortedWordList[lineNum]:
            readerPos = lineData[0] - 1
            preSpacesCount = lineData[1]
            spacesCount = lineData[2]
            length = lineData[3]
            formatType = lineData[4]
            ownFormat = lineData[5]
            isFormatStart = lineData[6]
            isFormatEnd = lineData[7]
            isWikiTagWord = lineData[8]
            word = escape(lineData[9])

            if x % onePercent == 0:
                stdout.write('.')
                stdout.flush()

            preRegEx1 = []
            preRegEx2 = []

            if isWikiTagWord:
                preRegEx1.append(r'[|]{0,1}')

            if isFormatStart:
                if ownFormat != -1:
                    preRegEx1.append(formats[ownFormat])
                    length += len(formatsReal[ownFormat])
                else:
                     preRegEx1.append(formats[formatType])
                     length += len(formatsReal[formatType])

            if preSpacesCount != 0:
                preRegEx1.append(r'[\s]{'+str(preSpacesCount)+r'}')
                length += preSpacesCount

            preRegEx1.append(word)

            if spacesCount != 0:
                preRegEx2.append(r'[\s]{'+str(spacesCount)+r'}')
                length += spacesCount

            if isFormatEnd:
                if ownFormat != -1:
                    preRegEx2.append(formats[ownFormat])
                    length += len(formatsReal[ownFormat])
                else:
                    preRegEx2.append(formats[formatType])
                    length += len(formatsReal[formatType])

            regExString = compile(''.join(preRegEx1)+''.join(preRegEx2))
            readerPos -= length

            if PRINTWORD and LINESTOPROCESS != 0 and lineNum == borderline:
                print()
                print(divider)
                print(lineNum, lineData)
                print(regExString.pattern)
                print(divider3)
                print(workLine)

            hasReplaced = False
            for match in finditer(regExString, workLine):
                findSpot = match.span()

                if readerPos <= findSpot[1] + FINDTOLERANCE:
                    if isWikiTagWord and workLine.find('|', findSpot[0]-1, findSpot[1]) != -1:
                        length += 1

                    if lineData[9] == ';':
                        tmpLine = workLine[findSpot[0]-10:findSpot[0]]
                        if tmpLine.find('&') != -1 and tmpLine.find(';') == -1: continue

                    workLine = workLine[:findSpot[0]] + (' ' * length)+ workLine[findSpot[1]:]
                    hasReplaced = True
                    break

            if not hasReplaced:
                if preSpacesCount != 0:
                    needle = r'[\s]{'+str(preSpacesCount)+r'}'

                    for index, item in enumerate(preRegEx1):
                        if item == needle:
                            del preRegEx1[index]
                            preRegEx1.insert(index-1, needle)
                            break

                if spacesCount != 0:
                    needle = r'[\s]{'+str(spacesCount)+r'}'

                    for index, item in enumerate(preRegEx2):
                        if item == needle:
                            del preRegEx2[index]
                            preRegEx2.insert(index+1, needle)
                            break

                regExString = compile(''.join(preRegEx1)+''.join(preRegEx2))

                for match in finditer(regExString, workLine):
                    findSpot = match.span()

                    if readerPos <= findSpot[1] + FINDTOLERANCE:
                        if isWikiTagWord and workLine.find('|', findSpot[0]-1, findSpot[1]) != -1:
                            length += 1

                        if lineData[9] == ';':
                            tmpLine = workLine[findSpot[0]-10:findSpot[0]]
                            if tmpLine.find('&') != -1 and tmpLine.find(';') == -1: continue

                        workLine = workLine[:findSpot[0]] + (' ' * length)+ workLine[findSpot[1]:]
                        break

                    """

                else:
                    # Debug
                    print()
                    print(regExString.pattern)
                    print('RP'+ str(readerPos), findSpot, length)
                    if ownFormat != -1: print(len(formatsReal[ownFormat]), formats[ownFormat], ownFormat)
                    elif formatType != -1: print(len(formatsReal[formatType]), formats[formatType], formatType)

                    print(lineNum + 1 , lineData)
                    print()
                    print(workLine)
                    exit()
                """

            if PRINTWORD and LINESTOPROCESS != 0 and lineNum == borderline:
                print(divider3)
                print(workLine)

            x += 1

        srcLineData[lineNum] = workLine
        purge()

    purge()
    return srcLineData

#-------------------------------------------------------------------------------
def removeEntities(srcLineData):
    print('\n\nRemoving entities...')

    with open(entityPath, 'r') as entityFile:

        x = 0
        for x, line in enumerate(entityFile):
            continue

        onePercent = floor(x / 100.0)
        if onePercent == 0: onePercent = 1

        entityFile.seek(0)

        divider = '#' * 90
        divider3 = '-' * 45

        x = 0

        for x, dataLine in enumerate(entityFile):
            if x % onePercent == 0:
                stdout.write('.')
                stdout.flush()

            lineData = dataLine.rstrip('\n').split('|', 9)
            position = int(lineData[0])
            lineNum = int(lineData[1]) - 1
            if LINESTOPROCESS != 0 and lineNum > LINESTOPROCESS: continue
            readerPos = int(lineData[2]) - 1
            preSpacesCount = int(lineData[3])
            spacesCount = int(lineData[4])
            formatType = int(lineData[5])
            ownFormat = int(lineData[6])
            isFormatStart = int(lineData[7])
            isFormatEnd = int(lineData[8])
            entity = escape(lineData[9])

            try:
                workLine = srcLineData[lineNum]
            except:
                continue

            preRegEx1 = []
            preRegEx2 = []
            length = len(lineData[9]);

            if preSpacesCount != 0:
                preRegEx1.append('[\s]{'+str(preSpacesCount)+'}')
                length += preSpacesCount

            if isFormatStart:
                if ownFormat != -1:
                    preRegEx1.append(formats[ownFormat])
                    length += len(formatsReal[ownFormat])
                else:
                    preRegEx1.append(formats[formatType])
                    length += len(formatsReal[formatType])

            preRegEx1.append(entity)


            if isFormatEnd:
                if ownFormat != -1:
                    preRegEx2.append(formats[ownFormat])
                    length += len(formatsReal[ownFormat])
                else:
                    preRegEx2.append(formats[formatType])
                    length += len(formatsReal[formatType])

            if spacesCount != 0:
                preRegEx2.append('[\s]{'+str(spacesCount)+'}')
                length += spacesCount

            regExString = compile(''.join(preRegEx1) + ''.join(preRegEx2))
            readerPos -= length

            if LINESTOPROCESS != 0 and lineNum + 1 == LINESTOPROCESS and PRINTENTITY:
                print()
                print(divider)
                print(lineNum, lineData)
                print(regExString.pattern)
                print(divider3)
                print(workLine)

            hasReplaced = False
            for match in finditer(regExString, workLine):
                findSpot = match.span()
                if readerPos <= findSpot[1] + FINDTOLERANCE:
                    srcLineData[lineNum] = workLine[:findSpot[0]] + (' ' * length) + workLine[findSpot[1]:]
                    hasReplaced = True
                    break

            if not hasReplaced:
                if preSpacesCount != 0:
                    needle = r'[\s]{'+str(preSpacesCount)+r'}'

                    for index, item in enumerate(preRegEx1):
                        if item == needle:
                            del preRegEx1[index]
                            preRegEx1.insert(index-1, needle)
                            break

                if spacesCount != 0:
                    needle = r'[\s]{'+str(spacesCount)+r'}'

                    for index, item in enumerate(preRegEx2):
                        if item == needle:
                            del preRegEx2[index]
                            preRegEx2.insert(index+1, needle)
                            break

                regExString = compile(''.join(preRegEx1)+''.join(preRegEx2))

                for match in finditer(regExString, workLine):
                    findSpot = match.span()

                    if readerPos <= findSpot[1] + FINDTOLERANCE:
                        srcLineData[lineNum] = workLine[:findSpot[0]] + (' ' * length)+ workLine[findSpot[1]:]
                        break

            if LINESTOPROCESS != 0 and lineNum + 1 == LINESTOPROCESS and PRINTENTITY:
                print(divider3)
                print(srcLineData[lineNum])

    purge()
    return srcLineData

#-------------------------------------------------------------------------------
def cleanupLeftOver(srcLineData):
    print('\n\nCleaning pipe characters, formatting and wikitag characters from src data...')
    onePercent = floor(len(srcLineData) / 100.0)
    if onePercent == 0: onePercent = 1
    x = 0

    for index, line in enumerate(srcLineData):
        if x % onePercent == 0:
            stdout.write('.')
            stdout.flush()

        if LINESTOPROCESS != 0 and index > LINESTOPROCESS: break

        workLine = srcLineData[index];
        for item in cleanupSequences:
            workLine = workLine.replace(item, '')

        srcLineData[index] = workLine

        x += 1


    return srcLineData

#----------------------------------------------------------------------------------------------------------------------------
def mergeWhiteSpace(srcLineData):
    print('\n\nMerging empty lines and whitespace...')
    onePercent = floor(len(srcLineData) / 100.0)
    if onePercent == 0: onePercent = 1

    for x, line in enumerate(srcLineData):
        if x % onePercent == 0:
            stdout.write('.')
            stdout.flush()

        line = line.lstrip()

        for match in finditer(r'[\s]{2,}', line):
            line = line.replace(match.group(), ' ..' + str(match.span()[1])+ '  ', 1)

        line = line.rstrip('\n')

        if len(line) != 0:
            outputData.append(line)
            outputDataLineNums.append(x)

    purge()
    return [outputData, outputDataLineNums]

#----------------------------------------------------------------------------------------------------------------------------
def writeShadowData(outputData, outputDataLineNums):
    print('\n\nWriting shadow data to disk...')
    with open(srcFilePath+'_shadow', 'w') as srcFile:
        for index, line in enumerate(outputData):
            srcFile.write(str(outputDataLineNums[index])+'\n'+line+'\n')

#----------------------------------------------------------------------------------------------------------------------------
def reportOrphands(outputData, outputDataLineNums):
    divider = '=' * 80;

    if DOSAVE:
        print('\n\n' + divider + '\nReporting orphands to file\n' +divider + '\n')

        if LINESTOPROCESS != 0:
            orphandFile = open('LimitedOrphandReport.txt', 'w')
        else:
            orphandFile = open('orphandReport.txt', 'w')
    else:
        print('\n\n' + divider + '\nReporting orphands\n' + divider + '\n')

    orphandCount = 0
    for index, line in enumerate(outputData):
        lineNum = outputDataLineNums[index]

        isReported = False
        for item in ascii_letters:
            if item in line:
                repDetail = 'Orphand strings in line ' + str(lineNum + 1) + ': '+ line + '\n'

                if DOSAVE: orphandFile.write(repDetail)
                else: print(repDetail)

                isReported = True
                orphandCount += 1
                break

        if not isReported:
            repDetail = 'Unknown orphand found in line ' + str(lineNum + 1) + ': '+ line + '\n'

            if DOSAVE: orphandFile.write(repDetail)
            else: print(repDetail)

            orphandCount += 1


    if DOSAVE:
        orphandFile.close()
        print('Reported ' + str(orphandCount) + ' orphands of ' + str(len(srcLineData)) + ' possible lines to file.\n' + divider)
    else:
        print('Reported ' + str(orphandCount) +' orphands of ' + str(len(srcLineData)) + ' possible lines.\n' + divider)

#----------------------------------------------------------------------------------------------------------------------------
# Start application
srcLineData = removeXMLTags(srcLineData)
srcLineData = removeWikiTags(srcLineData)
srcLineData = removeWords(srcLineData)
srcLineData = removeEntities(srcLineData)
srcLineData = cleanupLeftOver(srcLineData)
outputData = mergeWhiteSpace(srcLineData)
reportOrphands(outputData[0], outputData[1])
#writeShadowData(outputData[0], outputData[1])
print('Done. Have a good day!')
