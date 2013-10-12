#!/usr/bin/python

from optparse import OptionParser


parser = OptionParser()
parser.add_option("-t", type="int", dest="attr_num")
(option, args) = parser.parse_args()

#print option
#print args[0]

#s = "ihpcae/marshall_andrew/PIV/LSB/reacting/S = 0.58/1 atm/300 K/30 mps/100 H2/phi = 0.46/24 deg/Cam_Date=121103_Time=114443/B00004.im7"
#a = s.split("/")

s = args[0]
a = s.split("/")

def S():
    return a[5].split(" ")[2]

def Pressure():
    return a[6].split(" ")[0]

def Kelvin():
    return a[7].split(" ")[0]

def Velocity():
    return a[8].split(" ")[0]

def Composition():
    return a[9]

def Phi():
    return a[10].split(" ")[2]

def Temperature():
    return a[11].split(" ")[0]

def Date():
    return a[12].split("=")[1].split("_")[0]

def Time():
    return a[12].split("=")[2]

attr = {1:S, 2:Pressure, 3:Kelvin, 4:Velocity, 5:Composition, 6:Phi, 7:Temperature, 8:Date, 9:Time}

print attr[option.attr_num]()




