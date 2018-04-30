#!/usr/bin/python3
# -*- coding: utf-8 -*-
from re import escape, compile, search, sub, finditer, purge
from sys import stdout
from string import ascii_letters
from math import floor

# Constants
LIMIT = 0
PRINTWIKI = False
PRINTWORD = False
PRINTENITIES = False
DOSAVE = True

#-------------------------------------------------------------------------------
#srcFilePath = "enwiki-20160720-pages-meta-current1.xml-p000000010p000030303"
"""
srcFilePath = "data/enwik8"
wikiTagPath = 'data/wikitags.txt'
xmlTagPath = 'data/xmltags.txt'
wordPath = 'data/words.txt'
entityPath = 'data/entities.txt'
"""
srcFilePath = "data/enwiki8_small"
wikiTagPath = 'wikitags.txt'
xmlTagPath = 'xmltags.txt'
wordPath = 'words.txt'
entityPath = 'entities.txt'
# Have the line limit increase by one
#if LIMIT != 0: LIMIT += 1

# Internals
formats = ["'''''", "'''", "''", '======', '=====', '====', '===', '==']
tagOpenings = ['{{', '{|', '[[']
tagClosings = ['}}', '|}',  ']]']
cleanupSequences = ['|', '}', ']', '{', '[']
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
        if LIMIT != 0 and index > LIMIT:
            break
        srcLineData.append(line);

#-------------------------------------------------------------------------------
def removeWikiTags(srcLineData):
    print('\n\nRemoving wiki tags...')
    with open(wikiTagPath, 'r') as wikitagFile:
        x = 0
        for line in wikitagFile:
            x += 1

        onePercent = floor(x / 100.0)
        if onePercent == 0: onePercent = 1

        wikitagFile.seek(0)

        for x, dataLine in enumerate(wikitagFile):
            if x % onePercent == 0:
                stdout.write('.')
                stdout.flush()

            lineData = dataLine.rstrip('\n').split('|', 10)
            lineNum = int(lineData[0]) - 1
            position = int(lineData[1])
            preSpacesCount = int(lineData[2])
            spacesCount = int(lineData[3])
            tagType = int(lineData[4])
            formatType = int(lineData[5])
            ownFormat = int(lineData[6])
            isFormatStart = int(lineData[7])
            isFormatEnd = int(lineData[8])
            length = int(lineData[9])
            tag = escape(lineData[10])

            if LIMIT != 0 and lineNum > LIMIT:
                purge()
                return srcLineData

            preRegEx1 = []
            preRegEx2 = []

            workLine = srcLineData[lineNum]

            if preSpacesCount != 0:
                preRegEx1.append('[\s]{'+str(preSpacesCount)+'}')

            if isFormatStart:
                if ownFormat != -1: preRegEx1.append(formats[ownFormat])
                else: preRegEx1.append(formats[formatType])

            if tagType == 0:
                preRegEx1.append(tagOpenings[0])
                preRegEx2.append(tagClosings[0])
            elif tagType == 1:
                preRegEx1.append(tagOpenings[1])
                preRegEx2.append(tagClosings[1])
            else:
                preRegEx1.append(tagOpenings[2])
                preRegEx2.append(tagClosings[2])


            preRegEx1.append(r'[\s]{0,}' + tag)

            if isFormatEnd:
                if ownFormat != -1: preRegEx2.append(formats[ownFormat])
                else: preRegEx2.append(formats[formatType])

            if spacesCount != 0:
                preRegEx2.append('[\s]{'+str(spacesCount)+'}')

            regExString = compile(''.join(preRegEx1)+r'[\|]{0,1}')
            regExString2 = compile(''.join(preRegEx2))

            if LIMIT != 0 and lineNum == LIMIT and PRINTWIKI:
                print('##################################')
                print('-------------------------------------------------------------------------------')
                print(ownFormat, ownFormat == -1, isFormatStart, isFormatEnd, lineNum)
                print(lineData)
                print(regExString.pattern)
                print(regExString2.pattern)
                print('-----------------------------------------')
                print(workLine)

            workLine = regExString.sub(' ', workLine, 1)
            srcLineData[lineNum] = regExString2.sub(' ', workLine, 1).lstrip('|').lstrip()

            if LIMIT != 0 and lineNum == LIMIT and PRINTWIKI:
                print('-----------------------------------------')
                print(srcLineData[lineNum])

    purge()
    return srcLineData

#--------------------------------------------------------------------------------------------------------------------------
def removeXMLTags(srcLineData):
    print('\n\nCleaning xml tags...')

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

            if LIMIT != 0 and start > LIMIT:
                purge()
                return srcLineData

            workLine = srcLineData[start]
            regExString = compile(r'<'+tagName+r'[^>]{0,}>')
            srcLineData[start] = sub(regExString, ' ', workLine, 1)

            if LIMIT != 0:
                if end <= LIMIT:
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
        for line in wordFile:
            x += 1

            lineData = line.rstrip('\n').split('|', 9)
            lineNum = int(lineData[0]) - 1
            position = int(lineData[1])

            try:
                sortedWordList[lineNum]

                for index, entry in enumerate(lineData[2:]):
                    if index < 7:
                        lineData[index] = int(entry)
                    else:
                        lineData[index] = entry

                sortedWordList[lineNum].insert(position-1, lineData[:-2])
            except:
                sortedWordList[lineNum] = []

                for index, entry in enumerate(lineData[2:]):
                    if index < 7:
                        lineData[index] = int(entry)
                    else:
                        lineData[index] = entry

                sortedWordList[lineNum].insert(position-1, lineData[:-2])

    onePercent = floor(x / 100.0)
    if onePercent == 0: onePercent = 1
    x = 0

    for lineNum in sortedWordList.keys():
        if LIMIT != 0 and lineNum > LIMIT: return srcLineData
        workLine = srcLineData[lineNum].lstrip()

        for lineData in sortedWordList[lineNum]:
            preSpacesCount = lineData[0]
            spacesCount = lineData[1]
            length = lineData[2]
            formatType = lineData[3]
            ownFormat = lineData[4]
            isFormatStart = lineData[5]
            isFormatEnd = lineData[6]
            word = escape(lineData[7])

            if x % onePercent == 0:
                stdout.write('.')
                stdout.flush()

            preRegEx1 = []
            preRegEx2 = []

            if preSpacesCount != 0:
                preRegEx1.append('[\s]{'+str(preSpacesCount)+'}')

            if isFormatStart:
                if ownFormat != -1: preRegEx1.append(formats[ownFormat])
                else: preRegEx1.append(formats[formatType])

            if isFormatEnd:
                if ownFormat != -1: preRegEx2.append(formats[ownFormat])
                else: preRegEx2.append(formats[formatType])

            preRegEx1.append(word)

            if spacesCount != 0:
                preRegEx2.append('[\s]{'+str(spacesCount)+'}')

            regExString = compile(''.join(preRegEx1)+r'[\|]{0,1}'+''.join(preRegEx2))

            if LIMIT != 0 and lineNum == LIMIT and PRINTWORD:
                print('##################################')
                print('-------------------------------------------------------------------------------')
                print(ownFormat, ownFormat == -1, isFormatStart, isFormatEnd, lineNum)
                print(lineData)
                print(regExString.pattern)
                print('-----------------------------------------')
                print(workLine)
            for match in finditer(regExString, workLine):
                findSpot = match.span()
                workLine = workLine[:findSpot[0]] + workLine[findSpot[1]:]
                break;

            if LIMIT != 0 and lineNum == LIMIT and PRINTWORD:
                print('-----------------------------------------')
                print(workLine)

            x += 1

        srcLineData[lineNum] = workLine.lstrip('|').lstrip()
        purge()

    purge()
    return srcLineData

#-------------------------------------------------------------------------------
def removeEntities(srcLineData):
    print('\n\nRemoving entities...')

    with open(entityPath, 'r') as entityFile:
        x = 0
        for line in entityFile:
            x += 1

        onePercent = floor(x / 100.0)
        if onePercent == 0: onePercent = 1

        entityFile.seek(0)

        for x, dataLine in enumerate(entityFile):
            if x % onePercent == 0:
                stdout.write('.')
                stdout.flush()

            lineData = dataLine.rstrip('\n').split('|', 8)
            lineNum = int(lineData[0]) - 1
            if LIMIT != 0 and lineNum > LIMIT: continue
            position = int(lineData[1])
            preSpacesCount = int(lineData[2])
            spacesCount = int(lineData[3])
            formatType = int(lineData[4])
            ownFormat = int(lineData[5])
            isFormatStart = int(lineData[6])
            isFormatEnd = int(lineData[7])
            entity = escape(lineData[8])

            workLine = srcLineData[lineNum]

            preRegEx1 = []
            preRegEx2 = []

            if preSpacesCount != 0:
                preRegEx1.append('[\s]{'+str(preSpacesCount)+'}')

            if isFormatStart:
                if ownFormat != -1: preRegEx1.append(formats[ownFormat])
                else: preRegEx1.append(formats[formatType])

            if isFormatEnd:
                if ownFormat != -1: preRegEx2.append(formats[ownFormat])
                else: preRegEx2.append(formats[formatType])

            preRegEx1.append(entity)

            if spacesCount != 0:
                preRegEx2.append('[\s]{'+str(spacesCount)+'}')

            regExString = compile(''.join(preRegEx1) + ''.join(preRegEx2))

            if LIMIT != 0 and lineNum == LIMIT and PRINTENITIES:
                print('##################################')
                print('-------------------------------------------------------------------------------')
                print(ownFormat, ownFormat == -1, isFormatStart, isFormatEnd, lineNum)
                print(lineData)
                print(regExString.pattern)
                print('-----------------------------------------')
                print(workLine)

            for match in finditer(regExString, workLine):
                findSpot = match.span()
                srcLineData[lineNum] = workLine[:findSpot[0]] + workLine[findSpot[1]:]
                break;

            if LIMIT != 0 and lineNum == LIMIT and PRINTENITIES:
                print('-----------------------------------------')
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

        if LIMIT != 0 and index > LIMIT: break

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

        line = line.strip()
        for match in finditer(r'[\s]{2,}', line):
            line.replace(match.group(), '', 1)

        line = line.rstrip('\n')

        if len(line) != 0:
            outputData.append(line)
            outputDataLineNums.append(x)

    purge()
    return [outputData, outputDataLineNums]

#----------------------------------------------------------------------------------------------------------------------------
def writeShadowData(outputData, outputDataLineNums):
    print('\n\nWriting shadow data to disk...')
    #with open('enwiki-20160720-pages-meta-current1.xml-p000000010p000030303_shadow', 'w') as srcFile:
    with open(srcFilePath+'_shadow', 'w') as srcFile:
        for index, line in enumerate(outputData):
            srcFile.write(str(outputDataLineNums[index])+'\n'+line+'\n')

#----------------------------------------------------------------------------------------------------------------------------
def reportOrphads(outputData, outputDataLineNums):
    divider = '=' * 30;

    if DOSAVE:
        print('\n\n' + divider + '\nReporting orphands to file\n' +divider + '\n')

        if LIMIT != 0:
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
                repDetail = 'Orphand strings in line ' + str(lineNum) + ': '+ line + '\n'

                if DOSAVE: orphandFile.write(repDetail)
                else: print(repDetail)

                isReported = True
                orphandCount += 1
                break

        if not isReported:
            repDetail = 'Unknown orphand found in line ' + str(lineNum) + ': '+ line + '\n'

            if DOSAVE: orphandFile.write(repDetail)
            else: print(repDetail)

            orphandCount += 1


    if DOSAVE:
        orphandFile.close()
        print('Reported ' + str(orphandCount) + ' of ' + str(len(srcLineData)) + ' orphands to file.\n=========================================================================')
    else:
        print('Reporting ' + str(orphandCount) +' of ' + str(len(srcLineData)) + ' orphands.\n=========================================================================')

#----------------------------------------------------------------------------------------------------------------------------
# Start application
srcLineData = removeXMLTags(srcLineData)
srcLineData = removeWikiTags(srcLineData)
srcLineData = removeWords(srcLineData)
srcLineData = removeEntities(srcLineData)
outputData = mergeWhiteSpace(srcLineData)
reportOrphads(outputData[0], outputData[1])
#writeShadowData(outputData[0], outputData[1])
print('Done. Have a good day!')
