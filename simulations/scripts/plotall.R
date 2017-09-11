#!/usr/bin/env Rscript

# Sample usage (yes, truncate path after -m parameter)
# ./scripts/plotall.R ./data/r0.7-K1000-C0-s3000-a0.8-d1000-e10-p0-b20-c50-m



library(ggplot2)

source('scripts/theme_custom.R')

input=commandArgs(TRUE)[1]

if (exists("all_dfm")){
  rm(all_dfm)
}

# Function to compute the number of handovers given a flow start and end time
get_ho <- function(x) { nrow(subset(ho, ho$time > as.double(x["end_time"]) - as.double(x["elapsed"]) & ho$time < as.double(x["end_time"]))) }

for (mobility in c("gr", "anchor", "mapme")) {

  if (exists("dfm")){
    rm(dfm)
  }

  dir <- paste(input, mobility, "/1/", sep="")
  filenames <- Sys.glob(paste(dir, "flow_stats_elastic*", sep=""))
  for (file in filenames) {
      cat("file:\n")
      cat(file)
      cat("\n")
      temp_dfm <- read.table(paste(file, sep=""), header=TRUE);
      temp_dfm$down <- grepl("-down-", file);
      temp_dfm$mobility   <- gsub(".*-m([^/]*?)/.*", "\\1", file)
      if (!exists("dfm")){
          dfm <- temp_dfm
      } else {
          dfm<-rbind(dfm, temp_dfm)
      }
      rm(temp_dfm)
  }

  ho_file = paste(dir, "nHandovers.txt", sep="")
  if (!file.exists(ho_file))
    next

  ho <- try(read.table(ho_file))
  if (inherits(ho, 'try-error')) {
       dfm$num_handovers <- 0 
  } else {
      names(ho) <- c("time", "handover") #, "type")
      #subset(ho, ho$type == "D")    
      dfm$num_handovers <- apply(dfm, 1, get_ho)
  }

  # Merge all dfm into all_dfm
  if (!exists("all_dfm")){
      all_dfm <- dfm
  } else {
      all_dfm<-rbind(all_dfm, dfm)
  }

}

summary(ho)
summary(all_dfm);

a <- with(all_dfm, aggregate(all_dfm[, c("goodput", "elapsed", "num_holes", "num_timeouts", "rate")], list(num_handovers=num_handovers, mobility=mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

outfile <- paste("plots/goodput-", unlist(strsplit(input, "/"))[3], ".svg", sep="")

svg(outfile)
ggplot(a, aes(num_handovers, goodput[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=goodput[,"MEAN"]+goodput[,"CI"], ymax=goodput[,"MEAN"]-goodput[,"CI"])) + 
  xlab("Number of handovers") + 
  ylab("Mean flow goodput (Mb/s)") +
  theme_custom
dev.off()

outfile <- paste("plots/elapsed-", unlist(strsplit(input, "/"))[3], ".svg", sep="")

svg(outfile)
ggplot(a, aes(num_handovers, elapsed[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=elapsed[,"MEAN"]+elapsed[,"CI"], ymax=elapsed[,"MEAN"]-elapsed[,"CI"])) + 
  xlab("Number of handovers") + 
  ylab("Average flow completion time (s)") +
  theme_custom
dev.off()

outfile <- paste("plots/num_holes-", unlist(strsplit(input, "/"))[3], ".svg", sep="")

svg(outfile)
ggplot(a, aes(num_handovers, num_holes[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=num_holes[,"MEAN"]+num_holes[,"CI"], ymax=num_holes[,"MEAN"]-num_holes[,"CI"])) + 
  xlab("Number of holes") + 
  ylab("Average flow completion time (s)") +
  theme_custom
dev.off()


outfile <- paste("plots/num_timeouts-", unlist(strsplit(input, "/"))[3], ".svg", sep="")

svg(outfile)
ggplot(a, aes(num_handovers, num_timeouts[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=num_timeouts[,"MEAN"]+num_timeouts[,"CI"], ymax=num_timeouts[,"MEAN"]-num_timeouts[,"CI"])) + 
  xlab("Number of timeouts") + 
  ylab("Average flow completion time (s)") +
  theme_custom
dev.off()
