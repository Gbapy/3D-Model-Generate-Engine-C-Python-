import ctypes
import os
import sys
from ctypes import Structure
import pyau
class MeshInfo(Structure):
    _fields_ = [
        ("name", ctypes.c_char*256),
        ("vertCount", ctypes.c_int),
        ("meshCount", ctypes.c_int)]

class SizePattern(Structure):
    _fields_ = [
        ("gemSize", ctypes.c_double),
        ("prongSize", ctypes.c_double),
        ("holeSize", ctypes.c_double)]

cdll = ctypes.CDLL("/mnt/libGem.so")

######################################Getting function handles##############################

initFunc = cdll.initOperation
parseBmp = cdll.parseBmp
getGlbBufferSize = cdll.getGlbFileSize
getGlbAsBuffer = cdll.getGlbFileAsBuffer
getCurvedGemModelFromBuffer = cdll.getCurvedGemModelFromBuffer
deallocateMeshData = cdll.deallocateMeshData
deallocateBuffer = cdll.deallocateBuffer64

###########################################Define data types#################################

patSize = 6
CharPtr = ctypes.c_char_p
CharPtrPtr = ctypes.POINTER(CharPtr)
DblPtr = ctypes.POINTER(ctypes.c_double)
DblPtrPtr = ctypes.POINTER(DblPtr)
IntPtr = ctypes.POINTER(ctypes.c_int)
IntPtrPtr = ctypes.POINTER(IntPtr)
ExtraInfo = ctypes.c_double*14
ExtraInfoPtr = ctypes.POINTER(ExtraInfo)
MeshInfoPtr = ctypes.POINTER(MeshInfo)
MeshInfoPtrPtr = ctypes.POINTER(MeshInfoPtr)
Pattern = SizePattern * patSize
PatternPtr = ctypes.POINTER(Pattern)

######################################Define argument and return type##########################
initFunc.argtypes = [CharPtr, ctypes.c_int]
initFunc.restype = ctypes.c_int

parseBmp.argtypes = [CharPtr, IntPtr, IntPtr, CharPtrPtr]
parseBmp.restype = ctypes.c_int

getGlbBufferSize.argtypes = [DblPtr, DblPtr, IntPtr, MeshInfoPtr, ExtraInfoPtr]
getGlbBufferSize.restype = ctypes.c_int

getGlbAsBuffer.argtypes = [DblPtr, DblPtr, IntPtr, MeshInfoPtr, ExtraInfoPtr, CharPtr, ctypes.c_int]
getGlbAsBuffer.restype = ctypes.c_int

gemProngOption = ctypes.c_int(2); #1) no prong cutting + gem.  2) prong cutting + gem, 3) prong cutting + no gem

getCurvedGemModelFromBuffer.argtypes = [CharPtr, ctypes.c_int, ctypes.c_int, PatternPtr, 
                                        ctypes.c_int, ctypes.c_double, ctypes.c_double, ctypes.c_double, 
                                        DblPtrPtr, DblPtrPtr, IntPtrPtr, MeshInfoPtrPtr, ExtraInfoPtr, ctypes.c_int]
getCurvedGemModelFromBuffer.restype = ctypes.c_int

deallocateMeshData.argtypes = [DblPtr, DblPtr, IntPtr, MeshInfoPtr, ExtraInfoPtr]
deallocateMeshData.restype = ctypes.c_int

deallocateBuffer.argtypes = [ctypes.c_void_p, ctypes.c_int]
deallocateBuffer.restype = None

####################################Define variable########################################

initPath = "/mnt"
bmpPath = "/mnt/test.bmp"                                   
modelPath = "/mnt/out.glb"
rawFileData = ctypes.c_char_p(0)
rawFileSize = ctypes.c_int(0)
bmpWidth = ctypes.c_int(0)
bmpHeight = ctypes.c_int(0)
bmpData = ctypes.c_char_p(0)
extraInfo = ExtraInfo(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

vBuffer = DblPtr(ctypes.c_double(0.0))                      #Buffer for vertices
nBuffer = DblPtr(ctypes.c_double(0.0))                      #Buffer for normals
mBuffer = IntPtr(ctypes.c_int(0))                           #Buffer for triangles
mInfo = MeshInfoPtr(MeshInfo(bytes("Empty"), 0, 0))         #Information of model
thickness = ctypes.c_double(0.0013)                         #Thickness of model

#Define size parameters
pat = Pattern(SizePattern(0.0008, 0.0004, 0.00056), SizePattern(0.0009, 0.0004, 0.00063), SizePattern(0.001, 0.0004, 0.00075), 
              SizePattern(0.00115, 0.00043, 0.000805), SizePattern(0.0012, 0.00043, 0.00084), SizePattern(0.0013, 0.00045, 0.0009))
sizePattern = PatternPtr(pat)

####################################Do Main Operations#########################################

ret = initFunc(initPath, len(initPath))
if ret != 0:
    print("Fail to initialization!")
    print(ret)
    exit()

file_stats = os.stat(bmpPath)
rawFileSize = file_stats.st_size
file = open(bmpPath, "rb")  #Read Bmp File
rawFileData = file.read()
file.close()

#Parse bmp
ret = parseBmp(rawFileData, ctypes.byref(bmpWidth), ctypes.byref(bmpHeight), ctypes.byref(bmpData))
if ret != 0:
    exit()

bmpSize = ctypes.c_int(bmpWidth.value * bmpHeight.value * 3)

#Generate model and get mesh data
ret = getCurvedGemModelFromBuffer(bmpData, bmpWidth, bmpHeight, sizePattern, patSize, thickness, 0.0, 0.0,
                            ctypes.byref(vBuffer), ctypes.byref(nBuffer), ctypes.byref(mBuffer), 
                            ctypes.byref(mInfo), ctypes.byref(extraInfo), gemProngOption)

if ret != 0:
    exit()

#Deallocate Buffers
deallocateBuffer(rawFileData, rawFileSize)
deallocateBuffer(bmpData, bmpSize)

#Display Mesh Information
exInfo = ctypes.cast(extraInfo, ctypes.POINTER(ctypes.c_double))
volumn = exInfo[0]      #Volumn

mxx = exInfo[1]         #Maximum X
mxy = exInfo[2]         #Maximum Y
mxz = exInfo[3]         #Maximum Z

mnx = exInfo[4]         #Minimum X
mny = exInfo[5]         #Minimum Y
mnz = exInfo[6]         #Minimum Z

ringX1 = exInfo[7]      #X position of Ring1
ringY1 = exInfo[8]      #Y position of Ring1
ringZ1 = exInfo[9]      #Z position of Ring1
ringX2 = exInfo[10]     #X position of Ring2
ringY2 = exInfo[11]     #Y position of Ring2
ringZ2 = exInfo[12]     #Z position of Ring2

nodeCount = exInfo[13]  #Number of Node

print("Volumn = " + str(volumn))
print("Minimum Position = (" + str(mnx) + ", " + str(mny) + ", " + str(mnz) + ")")
print("Maximum Position = (" + str(mxx) + ", " + str(mxy) + ", " + str(mxz) + ")")
print("Ring1 Position = (" + str(ringX1) + ", " + str(ringY1) + ", " + str(ringZ1) + ")")
print("Ring2 Position = (" + str(ringX2) + ", " + str(ringY2) + ", " + str(ringZ2) + ")")
print("Number of Node = " + str(nodeCount))

#Get GLB file size
glbBufferSize = getGlbBufferSize(vBuffer, nBuffer, mBuffer, mInfo, extraInfo)
glbBuffer = ctypes.c_char * glbBufferSize
glbBuffer = glbBuffer('a')

#Get GLB file as buffer
ret = getGlbAsBuffer(vBuffer, nBuffer, mBuffer, mInfo, extraInfo, ctypes.cast(glbBuffer, ctypes.c_char_p), glbBufferSize)

#Deallocate buffers for mesh
deallocateMeshData(vBuffer, nBuffer, mBuffer, mInfo, extraInfo)    #Deallocating mesh data

if ret == 0:
    file = open(modelPath, "wb")
    file.write(glbBuffer) #Write GLB file
    file.close()
    deallocateBuffer(glbBuffer, glbBufferSize) #Deallocate buffer for GLB file


if ret != 0:
    print("Fail!")
else:
    print("Success!!!")



