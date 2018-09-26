"""
Run anomaly detection algorithms
Authors: Gyorgy Matyasfalvi (gmatyasfalvi@bnl.gov)
Create: August, 2018
"""

import sys
import time
import os
import configparser
import pickle
import numpy as np
np.set_printoptions(threshold=np.nan)
from sklearn.preprocessing import MinMaxScaler
import matplotlib as mpl
if os.environ.get('DISPLAY','') == '':
    print('no display found. Using non-interactive Agg backend')
    mpl.use('Agg')
import matplotlib.pyplot as plt
#import seaborn as sns
import outlier
import parser


# Proces config file
config = configparser.ConfigParser()
config.read(sys.argv[1])
pickleFile = config['Parser']['PickleFile']
dataType = config['Outlier']['DataType']
fixcontamination = int(config['Outlier']['FixContamination'])
contamination = float(config['Lof']['contamination'])
algorithm = config['Outlier']['Algorithm']


# This sets conp appropriately 
if fixcontamination > 0:
    conp = fixcontamination

# Load data (deserialize)
with open(pickleFile, 'rb') as handle:
    data = pickle.load(handle)

if dataType == 'funtime':
    funIds = list(data.keys())
    #===========================================================================
    # if funId in funIds:
    #     pass
    # else:
    #     raise Exception("Invalid function id ...")
    #===========================================================================
    for funId in funIds:
        if True: #funId == 23 or funId == 21:
            print("\n\n\nGenerating scatter plot for function: ", funId, " ...")
            
            # Save function data and Extract timestamp and exectuion time
            funData = X = np.array(data[funId])
            
            # Compute outliers
            otl = outlier.Outlier(sys.argv[1])
            otl.compOutlier(funData)
            scores = otl.getScore()
            outl = np.array(otl.getOutlier())
            
            # Number of data points
            numPoints = X.shape[0]
            
            # This sets conp appropriately
            if fixcontamination <= 0:
                conp = contamination * numPoints
        
            
            # Scale data
            scaler = MinMaxScaler()
            X = X[:,4:6]
            X = scaler.fit_transform(X)
            ymean = np.mean(X[:,1])
            ystd = np.std(X[:,1])
            
            
            if otl.getAlgorithm() == 'Lof' and numPoints > np.sum(outl) and numPoints > fixcontamination:
                
                numNeighbors = str(otl.getLofNumNeighbors())
                
                X = X[(-scores).argsort(kind='mergesort')]
                funData = funData[(-scores).argsort(kind='mergesort')]
                scores = scores[(-scores).argsort(kind='mergesort')]
                
                # Store scores 
                outfile = config['Parser']['OutFile'] + "NWChem_F" + str(funId) + "_Nn" + numNeighbors + "_scores.pickle"
                with open(outfile, 'wb') as handle:
                    pickle.dump(scores, handle, protocol=pickle.HIGHEST_PROTOCOL)
                    
                
                # Print funData
                #===============================================================
                # print("Anomaly info\n")
                # if fixcontamination > 0:
                #     for i in range(0,conp):
                #         print(i, ", ", funData[i,:])
                #===============================================================
                 
                # Plot with function ID 
                f = plt.figure()
                pltitle = "LOF Method on NWChem \n" + "(Function ID: " + str(funId) + ", Number of Data Points: " + str(numPoints) + ")" 
                
                main = plt.subplot(211)
                main.set_title(pltitle)
                main.set_xlabel('Function entry time (scaled)')
                main.set_ylabel('Function exec. time (scaled)')
                a = main.scatter(X[conp:, 0], X[conp:, 1], c='white', edgecolor='k', s=20) #sns.kdeplot(X[outl == 1, 0], X[outl == 1, 1], cmap="Blues", shade=True)
                b = main.scatter(X[:conp, 0], X[:conp, 1], c='red', edgecolor='k', s=20)
                #a = main.scatter(X[outl == 1, 0], X[outl == 1, 1], c='white', edgecolor='k', s=20)
                #b = main.scatter(X[outl == -1, 0], X[outl == -1, 1], c='red', edgecolor='k', s=20)
                main.axis('tight')
                main.set_xlim((-0.1, 1.1))
                main.set_ylim((-0.1, 1.1))
                
                # Create a legend
                main.legend([a, b], ["inlier", "outlier"], loc="upper center", bbox_to_anchor=(0.1, -0.4))
                
                if fixcontamination > 0:
                    params = main.annotate("nearest neighb. k: "+numNeighbors+"\n"+"contamination: "+" top "+str(fixcontamination), xy=(1, 0),  xycoords='data', 
                        xytext=(0.99, -0.5), textcoords='axes fraction',
                        bbox=dict(boxstyle="round", fc="w", alpha=0.25),
                        horizontalalignment='right', verticalalignment='top', multialignment='left')
                else:
                    params = main.annotate("nearest neighb. k: "+numNeighbors+"\n"+"contamination: "+" "+str(contamination*100)+"%", xy=(1, 0),  xycoords='data', 
                        xytext=(0.99, -0.5), textcoords='axes fraction',
                        bbox=dict(boxstyle="round", fc="w", alpha=0.25),
                        horizontalalignment='right', verticalalignment='top', multialignment='left')
                
                #params.set_alpha(.75)
                
                zoom = plt.subplot(212)
                zoom.set_title("Zoom")
                zoom.set_xlabel('Function entry time (scaled)')
                zoom.set_ylabel('Function exec. time (scaled)')
                a = zoom.scatter(X[conp:, 0], X[conp:, 1], c='white', edgecolor='k', s=20) #sns.kdeplot(X[outl == 1, 0], X[outl == 1, 1], cmap="Blues", shade=True)
                b = zoom.scatter(X[:conp, 0], X[:conp, 1], c='red', edgecolor='k', s=20)
                #a = zoom.scatter(X[outl == 1, 0], X[outl == 1, 1], c='white', edgecolor='k', s=20)
                #b = zoom.scatter(X[outl == -1, 0], X[outl == -1, 1], c='red', edgecolor='k', s=20)
                zoom.axis('tight')
                zoom.set_xlim((-0.1, 1.1))
                zoom.set_ylim(((ymean-(0.15*ystd)), (ymean+ystd)))
    
               
                plt.subplots_adjust(hspace=1.1)
                if fixcontamination > 0:
                    figfile = "NWChem_F" + str(funId) + "_" + "Nn" + numNeighbors + "_" + "ConTop" + str(fixcontamination) + ".png"
                else:
                    figfile = "NWChem_F" + str(funId) + "_" + "Nn" + numNeighbors + "_" + "Con" + str(contamination*100) + "%.png"
                    
                f.savefig(figfile)
            
            if otl.getAlgorithm() == 'Sstd' and numPoints > np.sum(outl):
                 
                # Plot with function ID 
                f = plt.figure()
                pltitle = "SSTD Method on NWChem \n" + "(Function ID: " + str(funId) + ", Number of Data Points: " + str(numPoints) + ")" 
                
                main = plt.subplot(211)
                main.set_title(pltitle)
                main.set_xlabel('Function entry time (scaled)')
                main.set_ylabel('Function exec. time (scaled)')
                a = main.scatter(X[outl == 1, 0], X[outl == 1, 1], c='white', edgecolor='k', s=20)
                b = main.scatter(X[outl == -1, 0], X[outl == -1, 1], c='red', edgecolor='k', s=20)
                main.axis('tight')
                main.set_xlim((-0.1, 1.1))
                main.set_ylim((-0.1, 1.1))
                
                # Create a legend
                main.legend([a, b], ["inlier", "outlier"], loc="upper center", bbox_to_anchor=(0.1, -0.4))
                sigma = str(otl.getSigma())
                
                if fixcontamination > 0:
                    params = main.annotate("Sigma: " + sigma + " contamination: "+" top "+str(fixcontamination), xy=(1, 0),  xycoords='data', 
                        xytext=(0.99, -0.5), textcoords='axes fraction',
                        bbox=dict(boxstyle="round", fc="w", alpha=0.25),
                        horizontalalignment='right', verticalalignment='top', multialignment='left')
                else:
                    params = main.annotate("Simga: " + sigma, xy=(1, 0),  xycoords='data', 
                        xytext=(0.99, -0.5), textcoords='axes fraction',
                        bbox=dict(boxstyle="round", fc="w", alpha=0.25),
                        horizontalalignment='right', verticalalignment='top', multialignment='left')
                
                #params.set_alpha(.75)
                
                zoom = plt.subplot(212)
                zoom.set_title("Zoom")
                zoom.set_xlabel('Function entry time (scaled)')
                zoom.set_ylabel('Function exec. time (scaled)')
                a = zoom.scatter(X[outl == 1, 0], X[outl == 1, 1], c='white', edgecolor='k', s=20)
                b = zoom.scatter(X[outl == -1, 0], X[outl == -1, 1], c='red', edgecolor='k', s=20)
                zoom.axis('tight')
                zoom.set_xlim((-0.1, 1.1))
                zoom.set_ylim(((ymean-(0.15*ystd)), (ymean+ystd)))
    
                plt.subplots_adjust(hspace=1.1)
                if fixcontamination > 0:
                    figfile = "NWChem_F" + str(funId) + "_" + "Sstd_Sigma"+sigma+".png"
                else:
                    figfile = "NWChem_F" + str(funId) + "_" + "Sstd_Sigma"+sigma+".png"
                    
                f.savefig(figfile)
        
