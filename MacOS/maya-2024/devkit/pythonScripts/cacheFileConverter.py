from __future__ import division
#-
# ==========================================================================
# Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
#
# Use of this software is subject to the terms of the Autodesk
# license agreement provided at the time of installation or download,
# or which otherwise accompanies this software in either electronic
# or hard copy form.
# ==========================================================================
#+

from builtins import object
from builtins import range
import os
import os.path
import getopt
import sys
import xml.dom.minidom
import string
import re
import array

"""

This example shows how to convert float channels found in cache files in Maya 8.5 and later to 
double channels, so that the cache file would then be compatible with the 
geometry cache in Maya 8.0.  It parses the XML file in addition to the cache data files and 
handles caches that are one file per frame as well as one file. 
To use: 

python cacheFileConverter.py -f mayaCacheFile.xml -o outputFileName


Overview of Maya Caches:
========================

Conceptually, a Maya cache consists of 1 or more channels of data.  
Each channel has a number of properties, such as:

- start/end time 
- data type of the channel (eg. "DoubleVectorArray" to represents a point array)
- interpretation (eg. "positions" the vector array represents position data, as opposed to per vertex normals, for example)
- sampling type (eg. "regular" or "irregular")
- sampling rate (meaningful only if sampling type is "regular")

Each channel has a number of data points in time, not necessarily regularly spaced, 
and not necessarily co-incident in time with data in other channels.  
At the highest level, a Maya cache is simply made up of channels and their data in time.

On disk, the Maya cache is made up of a XML description file, and 1 or more data files.  
The description file provides a high level overview of what the cache contains, 
such as the cache type (one file, or one file per frame), channel names, interpretation, etc.  
The data files contain the actual data for the channels.  
In the case of one file per frame, a naming convention is used so the cache can check its 
available data at runtime.

Here is a visualization of the data format of the OneFile case:

//  |---CACH (Group)	// Header
//  |     |---VRSN		// Version Number (char*)
//  |     |---STIM		// Start Time of the Cache File (int)
//  |     |---ETIM		// End Time of the Cache File (int)
//  |
//  |---MYCH (Group)	// 1st Time 
//  |     |---TIME		// Time (int)
//  |     |---CHNM		// 1st Channel Name (char*)
//  |     |---SIZE		// 1st Channel Size
//  |     |---DVCA		// 1st Channel Data (Double Vector Array)
//  |     |---CHNM		// n-th Channel Name
//  |     |---SIZE		// n-th Channel Size
//  |     |---DVCA		// n-th Channel Data (Double Vector Array)
//  |     |..
//  |
//  |---MYCH (Group)	// 2nd Time 
//  |     |---TIME		// Time
//  |     |---CHNM		// 1st Channel Name
//  |     |---SIZE		// 1st Channel Size
//  |     |---DVCA		// 1st Channel Data (Double Vector Array)
//  |     |---CHNM		// n-th Channel Name
//  |     |---SIZE		// n-th Channel Size
//  |     |---DVCA		// n-th Channel Data (Double Vector Array)
//  |     |..
//  |
//  |---..
//	|
//

In a multiple file caches, the only difference is that after the 
header "CACH" group, there is only one MYCH group and there is no 
TIME chunk.	In the case of one file per frame, the time is part of 
the file name - allowing Maya to scan at run time to see what data 
is actually available, and it allows users to move data in time by 
manipulating the file name.  

!Note that it's not necessary to have data for every channel at every time.  

"""

class CacheChannel(object):
    m_channelName = ""
    m_channelType = ""                
    m_channelInterp = ""
    m_sampleType = ""
    m_sampleRate = 0
    m_startTime = 0
    m_endTime = 0      
    def __init__(self,channelName,channelType,interpretation,samplingType,samplingRate,startTime,endTime):
        self.m_channelName = channelName
        self.m_channelType = channelType                
        self.m_channelInterp = interpretation
        self.m_sampleType = samplingType
        self.m_sampleRate = samplingRate
        self.m_startTime = startTime
        self.m_endTime = endTime             
        
class CacheFile(object):
    m_baseFileName = ""
    m_directory = ""   
    m_fullPath = "" 
    m_cacheType = ""
    m_cacheStartTime = 0
    m_cacheEndTime = 0
    m_timePerFrame = 0
    m_version = 0.0
    m_channels = []    
    ########################################################################
    #   Description:
    #       Class constructor - tries to figure out full path to cache
    #       xml description file before calling parseDescriptionFile()
    #
    def __init__(self,fileName):
        # fileName can be the full path to the .xml description file,
        # or just the filename of the .xml file, with or without extension
        # if it is in the current directory
        dir = os.path.dirname(fileName)
        fullPath = ""
        if dir == "":
            currDir = os.getcwd() 
            fullPath = os.path.join(currDir,fileName)
            if not os.path.exists(fullPath):
                fileName = fileName + '.xml';
                fullPath = os.path.join(currDir,fileName)
                if not os.path.exists(fullPath):
                    print("Sorry, can't find the file %s to be opened\n" % fullPath)
                    sys.exit(2)                    
                
        else:
            fullPath = fileName                
                
        self.m_baseFileName = os.path.basename(fileName).split('.')[0]        
        self.m_directory = os.path.dirname(fullPath)
        self.m_fullPath = fullPath
        self.parseDescriptionFile(fullPath)
        
    ########################################################################
    # Description:
    #   Writes a converted description file, where all instances of "FloatVectorArray"
    #   are replaced with "DoubleVectorArray"
    #
    def writeConvertedDescriptionFile(self,outputFileName):          
        newXmlFileName = outputFileName + ".xml"
        newXmlFullPath = os.path.join(self.m_directory,newXmlFileName)
        fd = open(self.m_fullPath,"r")
        fdOut = open(newXmlFullPath,"w")
        lines = fd.readlines()
        for line in lines:
            if line.find("FloatVectorArray") >= 0:
                line = line.replace("FloatVectorArray","DoubleVectorArray")
            fdOut.write(line)

    ########################################################################
    # Description:
    #   Given the full path to the xml cache description file, this 
    #   method parses its contents and sets the relevant member variables
    #
    def parseDescriptionFile(self,fullPath):          
        dom = xml.dom.minidom.parse(fullPath)
        root = dom.getElementsByTagName("Autodesk_Cache_File")
        allNodes = root[0].childNodes
        for node in allNodes:
            if node.nodeName == "cacheType":
                self.m_cacheType = node.attributes.item(0).nodeValue                
            if node.nodeName == "time":
                timeRange = node.attributes.item(0).nodeValue.split('-')
                self.m_cacheStartTime = int(timeRange[0])
                self.m_cacheEndTime = int(timeRange[1])
            if node.nodeName == "cacheTimePerFrame":
                self.m_timePerFrame = int(node.attributes.item(0).nodeValue)
            if node.nodeName == "cacheVersion":
                self.m_version = float(node.attributes.item(0).nodeValue)                
            if node.nodeName == "Channels":
                self.parseChannels(node.childNodes)
                
    ########################################################################
    # Description:
    #   helper method to extract channel information
    #            
    def parseChannels(self,channels):                         
        for channel in channels:
            if re.compile("channel").match(channel.nodeName) != None :
                channelName = ""
                channelType = ""                
                channelInterp = ""
                sampleType = ""
                sampleRate = 0
                startTime = 0
                endTime = 0                                               
                
                for index in range(0,channel.attributes.length):
                    attrName = channel.attributes.item(index).nodeName                                                            
                    if attrName == "ChannelName":                        
                        channelName = channel.attributes.item(index).nodeValue                        
                    if attrName == "ChannelInterpretation":
                        channelInterp = channel.attributes.item(index).nodeValue
                    if attrName == "EndTime":
                        endTime = int(channel.attributes.item(index).nodeValue)
                    if attrName == "StartTime":
                        startTime = int(channel.attributes.item(index).nodeValue)
                    if attrName == "SamplingRate":
                        sampleRate = int(channel.attributes.item(index).nodeValue)
                    if attrName == "SamplingType":
                        sampleType = channel.attributes.item(index).nodeValue
                    if attrName == "ChannelType":
                        channelType = channel.attributes.item(index).nodeValue
                    
                channelObj = CacheChannel(channelName,channelType,channelInterp,sampleType,sampleRate,startTime,endTime)
                self.m_channels.append(channelObj)
                    


def fileFormatError():
    print("Error: unable to read cache format\n");
    sys.exit(2)
    
def readInt(fd,needSwap):
    intArray = array.array('l')    
    intArray.fromfile(fd,1)
    if needSwap:    
        intArray.byteswap()
    return intArray[0]
    
def writeInt(fd,outInt,needSwap):
    intArray = array.array('l')    
    intArray.insert(0,outInt)   
    if needSwap:    
        intArray.byteswap()
    intArray.tofile(fd)

########################################################################
# Description:
#   method to parse and convert the contents of the data file, for the
#   One large file case ("OneFile")             
def parseDataOneFile(cacheFile,outFileName):
    dataFilePath = os.path.join(cacheFile.m_directory,cacheFile.m_baseFileName)
    dataFileNameOut = outFileName + ".mc"
    dataFilePathOut = os.path.join(cacheFile.m_directory,dataFileNameOut)
    dataFilePath = dataFilePath + ".mc"
    if not os.path.exists(dataFilePath):
        print("Error: unable to open cache data file at %s\n" % dataFilePath)
        sys.exit(2)
            
    fd = open(dataFilePath,"rb")
    fdOut = open(dataFilePathOut,"wb")
    
    blockTag = fd.read(4)
    fdOut.write(blockTag)
    
    #blockTag must be FOR4
    if blockTag != "FOR4":
        fileFormatError()
                
    platform = sys.platform
    needSwap = False
    if re.compile("win").match(platform) != None :
        needSwap = True
        
    if re.compile("linux").match(platform) != None :
        needSwap = True
        
    offset = readInt(fd,needSwap)    
    writeInt(fdOut,offset,needSwap)
    
    #The 1st block is the header, not used. 
    #just write out as is
    header = fd.read(offset)
    fdOut.write(header)
        
    while True:
        #From now on the file is organized in blocks of time
        #Each block holds the data for all the channels at that
        #time
        blockTag = fd.read(4)
        fdOut.write(blockTag)
        if blockTag == "":
            #EOF condition...we are done
            return
        if blockTag != "FOR4":
            fileFormatError()
        blockSize = readInt(fd,needSwap)
        
        #We cannot just write out the old block size, since we are potentially converting
        #Float channels to doubles, the block size may increase.
        newBlockSize = 0        
        bytesRead = 0
        #Since we don't know the size of the block yet, we will cache everything in a dictionary,
        #and write everything out in the end.
        blockContents = {}
        
        mychTag = fd.read(4)
        if mychTag != "MYCH":
            fileFormatError()
        bytesRead += 4        
        blockContents['mychTag'] = mychTag        
        
        timeTag = fd.read(4)
        if timeTag != "TIME":
            fileFormatError()
        bytesRead += 4           
        blockContents['timeTag']= timeTag        
        
        #Next 32 bit int is the size of the time variable,
        #this is always 4
        timeVarSize = readInt(fd,needSwap)
        bytesRead += 4
        blockContents['timeVarSize']= timeVarSize        
        
        #Next 32 bit int is the time itself, in ticks
        #1 tick = 1/6000 of a second
        time = readInt(fd,needSwap)            
        bytesRead += 4  
        blockContents['time']= time        
        
        newBlockSize = bytesRead
        channels = []
        blockContents['channels'] = channels
        
        print("Converting Data found at time %f seconds...\n"%(time/6000.0))            
        while bytesRead < blockSize:
            channelContents = {}
                                
            #channel name is next.
            #the tag for this must be CHNM
            chnmTag = fd.read(4)
            
            if chnmTag != "CHNM":
                fileFormatError()
            bytesRead += 4 
            newBlockSize += 4
            channelContents['chnmTag'] = chnmTag            
            
            #Next comes a 32 bit int that tells us how long the 
            #channel name is
            chnmSize = readInt(fd,needSwap)
            bytesRead += 4   
            newBlockSize += 4
            channelContents['chnmSize'] = chnmSize            
            
            #The string is padded out to 32 bit boundaries,
            #so we may need to read more than chnmSize
            mask = 3
            chnmSizeToRead = (chnmSize + mask) & (~mask)            
            channelName = fd.read(chnmSize)            
            paddingSize = chnmSizeToRead-chnmSize
            channelContents['channelName'] = channelName
            channelContents['paddingSize'] = paddingSize
            if paddingSize > 0:
                padding = fd.read(paddingSize)
                channelContents['padding'] = padding                
            
            bytesRead += chnmSizeToRead
            newBlockSize += chnmSizeToRead
            
            #Next is the SIZE field, which tells us the length 
            #of the data array
            sizeTag = fd.read(4)            
            channelContents['sizeTag'] = sizeTag
            if sizeTag != "SIZE":
                fileFormatError()
            bytesRead += 4  
            newBlockSize += 4
            
            #Next 32 bit int is the size of the array size variable,
            #this is always 4, so we'll ignore it for now
            #though we could use it as a sanity check.
            arrayVarSize = readInt(fd,needSwap)
            bytesRead += 4
            newBlockSize += 4
            channelContents['arrayVarSize'] = arrayVarSize            
            
            #finally the actual size of the array:
            arrayLength = readInt(fd,needSwap)            
            bytesRead += 4   
            newBlockSize += 4                  
            channelContents['arrayLength'] = arrayLength            
            
            #data format tag:
            dataFormatTag = fd.read(4)            
            #buffer length - how many bytes is the actual data
            bufferLength = readInt(fd,needSwap)                                    
            bytesRead += 8
            newBlockSize += 8
                        
            numPointsToPrint = 5
            if dataFormatTag == "FVCA":
                #FVCA == Float Vector Array
                outDataTag = "DVCA"                
                channelContents['dataFormatTag'] = outDataTag                
                
                if bufferLength != arrayLength*3*4:
                    fileFormatError()
                outBufLength = bufferLength*2                                    
                channelContents['bufferLength'] = outBufLength
                
                floatArray = array.array('f')    
                floatArray.fromfile(fd,arrayLength*3)
                doubleArray = array.array('d')
                bytesRead += arrayLength*3*4
                newBlockSize += arrayLength*3*8
                if needSwap:    
                    floatArray.byteswap()
                for index in range(0,arrayLength*3):    
                    doubleArray.append(floatArray[index])
                if needSwap:    
                    doubleArray.byteswap()
                channelContents['doubleArray'] = doubleArray
                channels.append(channelContents)                
                
            elif dataFormatTag == "DVCA":                
                #DVCA == Double Vector Array
                channelContents['dataFormatTag'] = dataFormatTag                
                if bufferLength != arrayLength*3*8:
                    fileFormatError()
                
                channelContents['bufferLength'] = bufferLength
                doubleArray = array.array('d')    
                doubleArray.fromfile(fd,arrayLength*3)
                
                channelContents['doubleArray'] = doubleArray
                channels.append(channelContents)
                bytesRead += arrayLength*3*8
                newBlockSize += arrayLength*3*8
                
            else:
                fileFormatError()                                    
        #Now that we have completely parsed this block, we are ready to output it
        writeInt(fdOut,newBlockSize,needSwap)        
        fdOut.write(blockContents['mychTag'])        
        fdOut.write(blockContents['timeTag'])        
        writeInt(fdOut,blockContents['timeVarSize'],needSwap)          
        writeInt(fdOut,blockContents['time'],needSwap)  
        for channelContents in channels:			
            fdOut.write(channelContents['chnmTag'])               
            writeInt(fdOut,channelContents['chnmSize'],needSwap)     
            fdOut.write(channelContents['channelName'])
            if channelContents['paddingSize'] > 0:                                            
                fdOut.write(channelContents['padding'])                
            fdOut.write(channelContents['sizeTag'])            
            writeInt(fdOut,channelContents['arrayVarSize'],needSwap)                           
            writeInt(fdOut,channelContents['arrayLength'],needSwap)                             
            fdOut.write(channelContents['dataFormatTag'])                             
            writeInt(fdOut,channelContents['bufferLength'],needSwap)             
            channelContents['doubleArray'].tofile(fdOut) 
            
            

########################################################################
# Description:
#   method to parse and convert the contents of the data file, for the
#   file per frame case ("OneFilePerFrame")             
def parseDataFilePerFrame(cacheFile,outFileName):    
    allFilesInDir = os.listdir(cacheFile.m_directory) 
    matcher = re.compile(cacheFile.m_baseFileName)
    dataFiles = []
    for afile in allFilesInDir:
        if os.path.splitext(afile)[1] == ".mc" and matcher.match(afile) != None:            
            dataFiles.append(afile)

    for dataFile in dataFiles:
        fileName = os.path.split(dataFile)[1]
        baseName = os.path.splitext(fileName)[0]
        
        frameAndTickNumberStr = baseName.split("Frame")[1]
        frameAndTickNumber = frameAndTickNumberStr.split("Tick")
        frameNumber = int(frameAndTickNumber[0])
        tickNumber = 0
        if len(frameAndTickNumber) > 1:
            tickNumber = int(frameAndTickNumber[1])
                        
        timeInTicks = frameNumber*cacheFile.m_timePerFrame + tickNumber
        print("--------------------------------------------------------------\n")      
        print("Converting data at time %f seconds:\n"%(timeInTicks/6000.0))        
        
        fd = open(dataFile,"rb")
        dataFileOut = outFileName + "Frame" + frameAndTickNumberStr + ".mc"
        dataFileOutPath = os.path.join(cacheFile.m_directory,dataFileOut)
        fdOut = open(dataFileOutPath,"wb")
        
        blockTag = fd.read(4)
        
        #blockTag must be FOR4
        if blockTag != "FOR4":
            fileFormatError()
        
        fdOut.write(blockTag)
                    
        platform = sys.platform
        needSwap = False
        if re.compile("win").match(platform) != None :
            needSwap = True
            
        if re.compile("linux").match(platform) != None :
            needSwap = True
            
        offset = readInt(fd,needSwap)        
        writeInt(fdOut,offset,needSwap)    
        
        #The 1st block is the header, not used. 
        #write out as is.
        header = fd.read(offset)
        fdOut.write(header)
        
        blockTag = fd.read(4)        
        if blockTag != "FOR4":
            fileFormatError()
        fdOut.write(blockTag)            
        blockSize = readInt(fd,needSwap)
        
        #We cannot just write out the old block size, since we are potentially converting
        #Float channels to doubles, the block size may increase.
        newBlockSize = 0        
        bytesRead = 0
        #Since we don't know the size of the block yet, we will cache everything in a dictionary,
        #and write everything out in the end.
        blockContents = {}                
        
        mychTag = fd.read(4)
        blockContents['mychTag'] = mychTag        
        if mychTag != "MYCH":
            fileFormatError()
        bytesRead += 4
        
        #Note that unlike the oneFile case, for file per frame there is no
        #TIME tag at this point.  The time of the data is embedded in the 
        #file name itself.        
        
        newBlockSize = bytesRead
        channels = []
        blockContents['channels'] = channels
                
        while bytesRead < blockSize:
            channelContents = {}
            #channel name is next.
            #the tag for this must be CHNM
            chnmTag = fd.read(4)
            if chnmTag != "CHNM":
                fileFormatError()
            bytesRead += 4                  
            newBlockSize += 4
            channelContents['chnmTag'] = chnmTag            
            
            #Next comes a 32 bit int that tells us how long the 
            #channel name is
            chnmSize = readInt(fd,needSwap)
            bytesRead += 4   
            newBlockSize += 4
            channelContents['chnmSize'] = chnmSize            
            
            #The string is padded out to 32 bit boundaries,
            #so we may need to read more than chnmSize
            mask = 3
            chnmSizeToRead = (chnmSize + mask) & (~mask)            
            channelName = fd.read(chnmSize)
            paddingSize = chnmSizeToRead-chnmSize
            channelContents['channelName'] = channelName
            channelContents['paddingSize'] = paddingSize
            if paddingSize > 0:
                padding = fd.read(paddingSize)
                channelContents['padding'] = padding                
            bytesRead += chnmSizeToRead
            newBlockSize += chnmSizeToRead
            
            #Next is the SIZE field, which tells us the length 
            #of the data array
            sizeTag = fd.read(4)
            channelContents['sizeTag'] = sizeTag
            if sizeTag != "SIZE":
                fileFormatError()
            bytesRead += 4  
            newBlockSize += 4
            
            #Next 32 bit int is the size of the array size variable,
            #this is always 4, so we'll ignore it for now
            #though we could use it as a sanity check.
            arrayVarSize = readInt(fd,needSwap)
            bytesRead += 4 
            newBlockSize += 4
            channelContents['arrayVarSize'] = arrayVarSize            
            
            #finally the actual size of the array:
            arrayLength = readInt(fd,needSwap)            
            bytesRead += 4                        
            newBlockSize += 4                  
            channelContents['arrayLength'] = arrayLength            
            
            #data format tag:
            dataFormatTag = fd.read(4)
            #buffer length - how many bytes is the actual data
            bufferLength = readInt(fd,needSwap)                    
            bytesRead += 8
            newBlockSize += 8
                        
            numPointsToPrint = 5
            if dataFormatTag == "FVCA":
                #FVCA == Float Vector Array
                outDataTag = "DVCA"                
                channelContents['dataFormatTag'] = outDataTag
                if bufferLength != arrayLength*3*4:
                    fileFormatError()
                    
                outBufLength = bufferLength*2                                    
                channelContents['bufferLength'] = outBufLength
                                
                floatArray = array.array('f')    
                floatArray.fromfile(fd,arrayLength*3)
                bytesRead += arrayLength*3*4
                newBlockSize += arrayLength*3*8
                doubleArray = array.array('d')
                                
                if needSwap:    
                    floatArray.byteswap()
                for index in range(0,arrayLength*3):    
                    doubleArray.append(floatArray[index])
                if needSwap:    
                    doubleArray.byteswap()
                channelContents['doubleArray'] = doubleArray
                channels.append(channelContents)                
                                                
            elif dataFormatTag == "DVCA":
                #DVCA == Double Vector Array
                channelContents['dataFormatTag'] = dataFormatTag
                if bufferLength != arrayLength*3*8:
                    fileFormatError()
                
                channelContents['bufferLength'] = bufferLength
                doubleArray = array.array('d')    
                doubleArray.fromfile(fd,arrayLength*3)
                channelContents['doubleArray'] = doubleArray
                channels.append(channelContents)
                bytesRead += arrayLength*3*8
                newBlockSize += arrayLength*3*8
                
            else:
                fileFormatError() 
                
        #Now that we have completely parsed this block, we are ready to output it
        writeInt(fdOut,newBlockSize,needSwap)        
        fdOut.write(blockContents['mychTag'])                
        for channelContents in channels:			
            fdOut.write(channelContents['chnmTag'])               
            writeInt(fdOut,channelContents['chnmSize'],needSwap)     
            fdOut.write(channelContents['channelName'])
            if channelContents['paddingSize'] > 0:                                            
                fdOut.write(channelContents['padding'])                
            fdOut.write(channelContents['sizeTag'])            
            writeInt(fdOut,channelContents['arrayVarSize'],needSwap)                           
            writeInt(fdOut,channelContents['arrayLength'],needSwap)                             
            fdOut.write(channelContents['dataFormatTag'])                             
            writeInt(fdOut,channelContents['bufferLength'],needSwap)             
            channelContents['doubleArray'].tofile(fdOut)                                 
    

def usage():
    print("Use -f to indicate the cache description file (.xml) you wish to convert\nUse -o to indicate the output filename")

try:
    (opts, args) = getopt.getopt(sys.argv[1:], "f:o:")
except getopt.error:
    # print help information and exit:
    usage()
    sys.exit(2)

if len(opts) != 2:
    usage()
    sys.exit(2)
        
fileName = ""
outFileName = ""
for o,a in opts:
    if o == "-f":
        fileName = a    
    if o == "-o":
        outFileName = a            

cacheFile = CacheFile(fileName)

if cacheFile.m_version > 2.0:
    print("Error: this script can only parse cache files of version 2 or lower\n")
    sys.exit(2)

print("Outputing new description file...\n")
cacheFile.writeConvertedDescriptionFile(outFileName)

print("Beginning Conversion of data files...\n")

if cacheFile.m_cacheType == "OneFilePerFrame":
    parseDataFilePerFrame(cacheFile,outFileName)
elif cacheFile.m_cacheType == "OneFile":
    parseDataOneFile(cacheFile,outFileName)
else:
    print("unknown cache type!\n")
    