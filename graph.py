import matplotlib.pyplot as plt

from optparse import OptionParser

parser = OptionParser()
parser.add_option("-e", type="int", dest="exp_num")
parser.add_option("-f", type="int", dest="func_num")
(option, args) = parser.parse_args()

i = args[0]
frame = i.split('i')
x = filter(lambda x: x != 'null' and x != '', frame[0].split(' '))
y = filter(lambda x: x != 'null' and x != '', frame[1].split(' '))
im1 = list(range(len(x)+1))[1:]

basepath = '/net/hu21/agangil3/experiments/'

def Plot():
  #image_path = '/net/hu21/agangil3/MediaKhan/khan' + '_experiment_id_' + str(option.attr_num)
  image_path = basepath + 'experiment_' + str(option.exp_num) +'_graph' 
  plt.plot(im1, x, 'b-', im1, y, 'r-')
  plt.savefig(image_path)

def Stats():
  f = open(basepath + 'experiment_' + str(option.exp_num) + '_stats.txt', 'w+')
  f.write(  'Max Frame1 Intensity: ' + str(max(x))        + '\n'
          + 'Max Frame2 Intensity: ' + str(max(y)) + '\n')
  f.close()        

attr = {1: Plot, 2: Stats}

attr[option.func_num]()  
