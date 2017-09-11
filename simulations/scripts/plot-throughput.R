#!/usr/bin/env Rscript

library(ggplot2)

source('scripts/theme_custom.R')

input=commandArgs(TRUE)[1]
#output=commandArgs(TRUE)[2]

filenames <- list.files(path=input, pattern="flow_stats.*")
if (exists("dfm")){
  rm(dfm)
}

for (file in filenames) {
    temp_dfm <- read.table(paste(input, file, sep=""), header=TRUE);
    temp_dfm$down <- grepl("-down-", file);
    if (!exists("dfm")){
        dfm <- temp_dfm
    } else {
        dfm<-rbind(dfm, temp_dfm)
    }
    rm(temp_dfm)
}

summary(dfm);

png("plots/goodput.png")
ggplot(dfm, aes(end_time, y=goodput, color=down)) + 
        geom_point() +
        xlab("Flow end time") +
        ylab("Flow goodput") +
        theme_custom
dev.off()

png("plots/num_holes.png")
ggplot(dfm, aes(end_time, y=num_holes, color=down)) + 
        geom_point() +
        xlab("Flow end time") +
        ylab("Flow num_holes") +
        theme_custom
dev.off()

png("plots/elapsed.png")
ggplot(dfm, aes(end_time, y=elapsed, color=down)) + 
        geom_point() +
        xlab("Flow end time") +
        ylab("Flow elapsed") +
        theme_custom
dev.off()

png("plots/rate.png")
ggplot(dfm, aes(end_time, y=rate, color=down)) + 
        geom_point() +
        xlab("Flow end time") +
        ylab("Flow rate") +
        theme_custom
dev.off()
