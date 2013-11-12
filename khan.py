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
#print 'Path is ', s
a = s.split("/")
#print "Split ", a

def S():
    for i in a:
      if 'S = ' in i:
        return i.split(" ")[2]

def Pressure():
    for i in a:
      if 'atm' in i:
        return i.split(" ")[0]

def Kelvin():
    for i in a:
      if 'K' in i:
        return i.split(" ")[0]

def Velocity():
    for i in a:
      if 'mps' in i:
        return i.split(" ")[0]

def Composition():
#    print 'called composition'
#    print a
    for i in a:
#      print i
      if 'H2' in i:
        return i
      elif 'CH4' in i:
        return i
        

def Phi():
    for i in a:
      if 'phi' in i:
        return i.split(" ")[2]

def Temperature():
    for i in a:
      if 'deg' in i:
        return i.split(" ")[0]

def Date():
    for i in a:
      if 'Cam_Date' in i:
        return i.split("=")[1].split("_")[0]

def Time():
    for i in a:
      if 'Cam_Date' in i:
        return i.split("=")[2]

attr = {1:S, 2:Pressure, 3:Kelvin, 4:Velocity, 5:Composition, 6:Phi, 7:Temperature, 8:Date, 9:Time}

print attr[option.attr_num]()




