#!/usr/bin/env Rscript

library(ggplot2)

source('scripts/theme_custom.R')

input=commandArgs(TRUE)[1]

filenames <- list.files(path=input, pattern="flow_stats.*")
if (exists("dfm")){
  rm(dfm)
}

get_ho <- function(x) { nrow(subset(ho, ho$time > as.double(x["end_time"]) - as.double(x["elapsed"]) & ho$time < as.double(x["end_time"]))) }

for (file in filenames) {
    temp_dfm <- read.table(paste(input, file, sep=""), header=TRUE);
    temp_dfm$down <- grepl("-down-", file);
    temp_dfm$mobility   <- gsub(".*-m([^/]*?)/.*", "\\1", file)
    if (!exists("dfm")){
        dfm <- temp_dfm
    } else {
        dfm<-rbind(dfm, temp_dfm)
    }
    rm(temp_dfm)
}


ho <- try(read.table(paste(input, "nHandovers.txt", sep="")));
if (inherits(ho, 'try-error')) {
     dfm$num_handovers <- 0 
} else {
    names(ho) <- c("time", "handover") #, "type")
    #subset(ho, ho$type == "D")    
    dfm$num_handovers <- apply(dfm, 1, get_ho)
}


summary(dfm);

a <- with(dfm, aggregate(dfm[, c("goodput", "elapsed", "num_holes", "rate")], list(num_handovers=num_handovers, mobility=mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

outfile <- paste("plots/goodput-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(a, aes(num_handovers, goodput[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=goodput[,"MEAN"]+goodput[,"CI"], ymax=goodput[,"MEAN"]-goodput[,"CI"])) + 
  xlab("Number of handovers") + 
  ylab("Mean flow goodput (Mb/s)") +
  theme_custom_tl
dev.off()

outfile <- paste("plots/num_timeouts-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(a, aes(num_handovers, num_timeouts[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=num_timeouts[,"MEAN"]+num_timeouts[,"CI"], ymax=num_timeouts[,"MEAN"]-num_timeouts[,"CI"])) + 
  xlab("Number of handovers") + 
  ylab("Mean flow num_timeouts (Mb/s)") +
  theme_custom_tl
dev.off()

