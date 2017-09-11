import sys

mapSize = 2100
APdistance = 80

row = APdistance/2
while row < mapSize:
    column = APdistance/2
    while column < mapSize:
        print str(row)+" "+str(column)+" 0"
        column+=APdistance
    if (column-mapSize)< APdistance/2:
        #additional column
        print str(row)+" "+str(mapSize-1)+" 0"
    row+=APdistance

if (row-mapSize)< APdistance/2:
    column = APdistance/2
    while column < mapSize:
        print str(mapSize-1)+" "+str(column)+" 0"
        column+=APdistance
    if (column-mapSize)< APdistance/2:
        #additional column
        print str(mapSize-1)+" "+str(mapSize-1)+" 0"