#!/usr/bin/python

from optparse import OptionParser

import sys
sys.path.append('./libim7/libim7')

import numpy as np
import libim7.libim7 as im7
import matplotlib.pyplot as plt
from pylab import *
import glob

y_frame0 = []
y_frame1 = []

parser = OptionParser()
parser.add_option("-t", type="int", dest="attr_num")
(option, args) = parser.parse_args()

#print option
#print args[0]

#s = "ihpcae/marshall_andrew/PIV/LSB/reacting/S = 0.58/1 atm/300 K/30 mps/100 H2/phi = 0.46/24 deg/Cam_Date=121103_Time=114443/B00004.im7"
#a = s.split("/")

s = "/net/hu21/agangil3/data/PIV/LSB/reacting/S = 0.58/1 atm/300 K/30 mps/100 H2/phi = 0.46/0 deg/Cam_Date=121103_Time=112726/B00001.im7" 

#s = args[0]
#print 'Path is ', s
a = s.split("/")
#print "Split ", a

buf, att = ('', '')

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
    for i in a:
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

def Intensity_Frame1():
  buf, att = im7.readim7(s)
  return mean(buf.get_frame(0))

def Intensity_Frame2():
  buf, att = im7.readim7(s)
  return mean(buf.get_frame(1))

def mean(img):
  sum_intensity = 0
  for row in range (len(img)):
    sum_intensity += sum(img[row])
  total_pixels = len(img) * len(img[0])
  return sum_intensity / total_pixels


attr = {1:S, 2:Pressure, 3:Kelvin, 4:Velocity, 5:Composition, 6:Phi, 7:Temperature, 8:Date, 9:Time, 10:Intensity_Frame1, 11:Intensity_Frame2}

print attr[option.attr_num]()




