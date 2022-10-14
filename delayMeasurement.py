import midas.file_reader
import dsproto_vx2740.dump_vx2740_data
import matplotlib.pyplot as plt
import scipy as sp
import numpy as np
import argparse
from scipy import optimize

def phaseParser():
    parser = argparse.ArgumentParser(description='Read data from midas file (in .lz4 format) to calculate phase between the clock of VX1 and VX2')
    parser.add_argument('fileName', type=str, help = 'Name of the file we want to read data from (Example: run00389.mid.lz4)')
    parser.add_argument('numberEvents', type=int, help = 'Number of events recorded in the file')
    parser.add_argument('numberVX', type=int, help = 'Number of VX used in this run (usually 2...)')
    parser.add_argument('sizeEvents', type=int, help = 'Number of points per event')
    parser.add_argument('stopEvent', type=int, help = 'Number of events you want to go through to calculate phase')
    parser.add_argument('minHist', type=int, help = 'Minimal value for the x axis of the phase measurement histogram (in ns)')
    parser.add_argument('maxHist', type=int, help = 'Maximal value for the x axis of the phase measurement histogram (in ns)')
    parser.add_argument('numberBin', type = int, help = 'Number of bins wanted for the generated histogram')
    parser.add_argument('writeToTXT', type = str, help = 'Write argument as yes to generate text file with results of calculation')
    parser.add_argument('saveAsPDF', type = str, help = 'Save generated plots to PDF files')
    global args
    args = parser.parse_args()
    return
def fileRead (filePath, numberEvents, numberVX, sizeEvents, stopEvent):
    
    
    global phase, amplitude, trigTime, board1, board2, period1, period2, numEvents, numVX, sizeEv, waveform1, waveform2, offset1, offset2, amplitude1, amplitude2, fit1, fit2, phase1, phase2, popt1, popt2, vx
    
    
    numVX = numberVX
    sizeEv = sizeEvents
    phase = np.zeros([numberVX, numberEvents])
    amplitude = np.empty([numberVX, numberEvents])
    trigTime = np.empty([numberVX, numberEvents])
    board1 = np.empty([numberEvents, sizeEvents])
    board2 = np.empty([numberEvents, sizeEvents])
    flag1 = flag2 = 0
    
    
    midas_file = midas.file_reader.MidasFile(filePath)
    for ev in midas_file:
        all_vx_data = dsproto_vx2740.dump_vx2740_data.midas_to_vx2740(ev)
        if all_vx_data is not None:
    	
            for vx in all_vx_data:
                if vx.format == 0x10:
    				
                    if vx.board_id == 0:
                        board1[vx.event_counter] = np.array(vx.waveforms.get(0))
                        waveform1 = vx.waveforms.get(0)
                        amplitude1 = (np.amax(waveform1)-np.amin(waveform1))/2
                        offset1 = (np.amax(waveform1)+np.amin(waveform1))/2
                        phase1 = np.where(np.diff(np.sign(waveform1-offset1)))[0][0]		
                        half = round(np.size(waveform1)/2)
                        period1 = np.size(waveform1)/(np.argmax(np.abs(np.fft.fft(waveform1-offset1)[0:half])))			
                        popt1= sp.optimize.curve_fit(sine, np.arange(0, np.size(waveform1), 1),waveform1,[offset1,amplitude1,period1,phase1])
                        fit1=sine(np.arange(0, sizeEv, 1),*popt1[0])
                        phase[0,vx.event_counter] = popt1[0][3]
                        amplitude[0,vx.event_counter] = popt1[0][1]
                        trigTime[0,vx.event_counter] = vx.trigger_time_ticks
        

                        if vx.event_counter > stopEvent:
                            flag1 =1
    				
                    if vx.board_id == 1:
                        board2[vx.event_counter] = np.array(vx.waveforms.get(0))
                        waveform2 = vx.waveforms.get(0)
                        amplitude2 = (np.amax(waveform2)-np.amin(waveform2))/2
                        offset2 = (np.amax(waveform2)+np.amin(waveform2))/2
                        phase2 = np.where(np.diff(np.sign(waveform2-offset2)))[0][0]
                        half = round(np.size(waveform2)/2)
                        period2 = np.size(waveform2)/(np.argmax(np.abs(np.fft.fft(waveform2-offset2)[0:half])))					
                        popt2,pcov2 = sp.optimize.curve_fit(sine, np.arange(0, np.size(waveform2), 1),waveform2,[offset2,amplitude2,period2,phase2])
                        fit2=sine(np.arange(0, sizeEv, 1),*popt2)
                        phase[1,vx.event_counter] = popt2[3]                    
                        amplitude[1,vx.event_counter] = popt2[1]
                        trigTime[1,vx.event_counter] = vx.trigger_time_ticks
                        
                        
                        if vx.event_counter > stopEvent:
                            flag2 = 1

                    
        if flag1 == 1 and flag2 == 1:
            break
    numEvents = vx.event_counter
    sizeEv = np.size(vx.waveforms.get(0))
    return
def sine (x,offset,amplitude,period,phase):
 	return 	offset + amplitude * np.sin(2*np.pi*np.arange(0, sizeEv)/period-2*np.pi*(phase/period))
def phaseMeas():
    global trigDelta,delta
    trigDelta = np.empty(numEvents) 
    delta = np.empty(numEvents)
    
 	
    for i in range(numEvents):
    
#        print(i,trigTime[0,i], trigTime[1,i])
    
        if np.sign(amplitude[0,i]) != np.sign(amplitude[1,i]):
            if np.sign(amplitude[0,i]) == -1 and np.sign(amplitude[1,i]) == 1:
                phase[0,i] = phase[0,i] + period2/2
                if (phase[1,i]-phase[0,i]) > 0.99*period2:
                      phase[1,i] = phase[1,i] - period2
            if np.sign(amplitude[0,i]) == 1 and np.sign(amplitude[1,i]) == -1:
                phase[1,i] = phase[1,i] + period2/2
                if (phase[1,i]-phase[0,i]) > 0.99*period2:
                      phase[1,i] = phase[1,i] - period2
            
        trigDelta[i] = trigTime[1,i]-trigTime[0,i]
        
    
        
        if trigDelta[i] == 0:
            delta [i] = (phase[1,i]-phase[0,i]) * 8.0
        else:
            delta[i] = 9999
            
    return
def gaussian(x,amp,mean,std):
    y_out = amp*1/(std * np.sqrt(2 * np.pi)) * np.exp( - (x - mean)**2 / (2 * std**2))
    return y_out


def plotsAndNumbers():
    min_hist=args.minHist
    max_hist=args.maxHist
    bin_hist=args.numberBin
    
    print("bin size = {:.3f} ns".format((max_hist-min_hist)/bin_hist))
    bins = plt.hist(delta, bins = bin_hist, range = (min_hist,max_hist))[1]
    heights = plt.hist(delta, bins = bin_hist, range = (min_hist,max_hist))[0]
    centerbin = np.zeros(np.size(bins)-1)
    for i in range(0, np.size(bins)-1, 1):
        centerbin[i] = (bins[i]+bins[i+1])/2
    #num_bins = np.size(heights)
    num_events = np.sum(heights)
    print("num_events = ",num_events)
    mean = np.sum(centerbin*heights)/num_events
    print("mean = {:.3f} ns".format(mean))

    sum0 = 0
    sum2 = 0
    for i in range(0, np.size(centerbin)):
        s = (centerbin[i]-mean)*heights[i]
        # print(i, mean, s)
        sum0 += heights[i]
        sum2 += s * s
        
    
    
    
    rms = np.sqrt(sum2/sum0)
    print("rms = {:.3f} ns".format(rms))
    
    mean_error = rms/np.sqrt(num_events)
    print("mean_error = {:.3f} ns".format(mean_error))
    
    
    
    popt3,pcov3 = sp.optimize.curve_fit(gaussian, centerbin, heights, [np.max(heights),mean,np.std(bins)])
    plt.plot(np.linspace(min_hist,max_hist,sizeEv), gaussian(np.linspace(min_hist,max_hist,sizeEv), popt3[0],popt3[1],popt3[2]))
    
    centroid = (np.linspace(min_hist,max_hist,sizeEv))[np.argmax(gaussian(np.linspace(min_hist,max_hist,sizeEv), popt3[0],popt3[1],popt3[2]))]
    sigma = np.sqrt(2*np.log(2))*popt3[2]
    std_error = (popt3[2]/np.sqrt(num_events))
    if num_events > 10:
        popt3,pcov3 = sp.optimize.curve_fit(gaussian, centerbin, heights, [np.max(heights),mean,np.std(bins)])
        plt.plot(np.linspace(min_hist,max_hist,sizeEv), gaussian(np.linspace(min_hist,max_hist,sizeEv), popt3[0],popt3[1],popt3[2]))
        centroid = (np.linspace(min_hist,max_hist,sizeEv))[np.argmax(gaussian(np.linspace(min_hist,max_hist,sizeEv), popt3[0],popt3[1],popt3[2]))]
        sigma = np.sqrt(2*np.log(2))*popt3[2]
        std_error = (popt3[2]/np.sqrt(num_events))
        
        print("centroid = {:.3f} ns.".format(centroid))
        print("width (sigma) = {:.3f} ns.".format(sigma))
        print("error on the centroid = {:.6f} ns.".format(std_error))
        
        # xxx = 8

        # plt.text(x = xxx, y = 10, s = "num_events = {}".format(num_events),size = 13)
        # plt.text(x = xxx, y = 9.5, s = "bin size = {:.3f} ns".format((max_hist-min_hist)/bin_hist),size = 13)
        # plt.text(x = xxx, y = 9, s = "mean = {:.3f} ns".format(mean),size = 13)
        # plt.text(x = xxx, y = 8.5, s = "rms = {:.3f} ns.".format(rms),size = 13)
        # plt.text(x = xxx, y = 8, s = "mean_error = {:.3f} ns.".format(mean_error),size = 13) 
        # plt.text(x = xxx, y = 5, s =  "centroid = {:.3f} ns.".format(centroid),size = 13)
        # plt.text(x = xxx, y = 4.5, s =  "width (sigma) = {:.3f} ns.".format(sigma),size = 13)
        # plt.text(x = xxx, y = 4, s =  "error on the centroid = {:.6f} ns.".format(std_error),size = 13)
    
    if args.writeToTXT == 'yes':
        f = open(args.fileName+".txt","w+")
        f.write("num_events = {}\n".format(num_events))
        f.write("bin size = {:.3f} ns\n".format((max_hist-min_hist)/bin_hist))
        f.write("mean = {:.3f} ns\n".format(mean))
        f.write("rms = {:.3f} ns\n".format(rms))
        f.write("mean_error = {:.3f} ns\n".format(mean_error))
        f.write("centroid = {:.3f} ns\n".format(centroid))
        f.write("width (sigma) = {:.3f} ns\n".format(sigma))
        f.write("error on the centroid = {:.6f} ns\n".format(std_error))
        f.close()
    
    plt.title("Phase between VX1-VX2 and gaussian fit", fontsize = 20)
    plt.ylabel("Number of events", fontsize = 16)
    plt.xlabel("Phase in ns", fontsize = 16)
    plt.yticks(fontsize=14)
    plt.xticks(fontsize=14)
    plt.show()
    if args.saveAsPDF== 'yes':
        plt.savefig(args.fileName+'_Histogram&Gaussian.pdf')  
    
    
    
    
    plt.figure()
    plt.plot(delta)
    plt.title("Phase difference between VX1 and VX2 for each event (in seconds)")
    plt.ylabel("Phase difference in seconds")
    plt.ylim(min_hist,max_hist)
    plt.xlabel("Event number")
    plt.show()
    if args.saveAsPDF== 'yes':
        plt.savefig(args.fileName+'_PhaseGraph.pdf')    
    
    plt.figure()
    plt.plot(trigDelta)
    plt.title("Difference between VX1 and VX2 trigger time stamps")
    plt.ylabel("Difference in trigger time stamps")
    plt.xlabel("Event number")
    plt.show()
    if args.saveAsPDF== 'yes':
        plt.savefig(args.fileName+'_TriggerDelta.pdf')      
    
    
    # making subplots
    
    fig, ax = plt.subplots(3, 1)
     
    # set data with subplots and plot
    ax[0].plot(waveform1, color = "blue")
    ax[0].title.set_text("VX2740 Waveform")
    ax[1].plot(offset1 + amplitude1 * np.sin(2*np.pi*np.arange(0, np.size(waveform1), 1)/period1-2*np.pi*(phase1/period1)), color = "orange")
    ax[1].title.set_text("First approximation")
    ax[2].plot(fit1, color = "green")
    ax[2].title.set_text("Fit")
    fig.tight_layout(pad=2.5)
    plt.show()
    if args.saveAsPDF== 'yes':
        plt.savefig(args.fileName+'_Waveform.pdf')      
    return

phaseParser()
fileRead("/zssd/home1/vx_napoli/online/data/"+args.fileName,args.numberEvents,args.numberVX,args.sizeEvents,args.stopEvent)
phaseMeas()
plotsAndNumbers()