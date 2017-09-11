#!/usr/bin/env Rscript

#example of usage:
#for producing plots of one consumer/producer pairs:
# ./show-mobile-delay.R data/synthetic-r0.5-f0-F0-S1-E0-N1

library(ggplot2)
require(gridExtra)
input=commandArgs(TRUE)[1]
inputPattern<-paste(input,"*/*/producer_handover_stats-*",sep="")
rateTraceFiles <- Sys.glob(inputPattern)
print("number of trace files is")
print(length(rateTraceFiles))


ncouples<-gsub(".*-N([^/]*?)/.*", "\\1", rateTraceFiles[1])


if (exists("dfm")){
  rm(dfm)
}


for(rateTraceFile in rateTraceFiles){

rateData<-read.table(rateTraceFile,header=T)
rateData$Mobility<-gsub(".*-m([^/]*?)/.*", "\\1", rateTraceFile)


 if (!exists("dfm")){
        dfm <- rateData
    }
 else{
 dfm<-rbind(dfm,rateData)
 }


print(gsub(".*-m([^/]*?)/.*", "\\1", rateTraceFile))
rm(rateData)
}

dfm$Mobility<-as.factor(dfm$Mobility)
#dfm<-subset(dfm, Mobility != "kite")



plot2<-ggplot(dfm, aes(l3_handoff_time, colour = Mobility)) + stat_ecdf(show.legend=T)+xlab("layer 3 handoff latency [s]")+ylab(paste("cdf of layer3 handoff latency with ", as.character(ncouples),"consumer/producer pairs", sep=""))+theme_bw()
#+coord_cartesian(xlim=c(0,0.2))
# uncomment the above line to zoom in the interval of (0,0.2)s

#X11()
png(paste("~/Documents/cdf_of_hanodff_delay.png"))
print(plot2)
#Sys.sleep(100000)
dev.off()
