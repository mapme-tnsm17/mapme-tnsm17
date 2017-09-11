#!/usr/bin/env Rscript

# Plot streaming flow statistics
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
  filenames <- Sys.glob(paste(dir, "flow_stats_streaming*", sep=""))
  for (file in filenames) {
#cat("file:\n")
#cat(file)
#cat("\n")
      temp_dfm <- read.table(paste(file, sep=""), header=TRUE);
      temp_dfm$down <- grepl("-down-", file);
      temp_dfm$mobility   <- gsub(".*-m([^/]*?)/.*", "\\1", file)
#print(summary(temp_dfm))
      if (!exists("dfm")){
          dfm <- temp_dfm
      } else {
          dfm<-rbind(dfm, temp_dfm)
      }
      rm(temp_dfm)
#print(summary(dfm))
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

all_dfm$loss_rate = all_dfm$loss_rate * 100
summary(all_dfm);

a <- with(all_dfm, aggregate(all_dfm[, c("elapsed", "delay", "loss_rate")], list(num_handovers=num_handovers, mobility=mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

summary(a)
print(a$elapsed[,"MEAN"])

print(a)

outfile <- paste("plots/elapsed-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(a, aes(num_handovers, elapsed[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=elapsed[,"MEAN"]+elapsed[,"CI"], ymax=elapsed[,"MEAN"]-elapsed[,"CI"])) + 
  xlab("Number of handovers") + 
  ylab("Mean flow completion time (s)") +
  theme_custom
dev.off()

outfile <- paste("plots/delay-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(a, aes(num_handovers, delay[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=delay[,"MEAN"]+delay[,"CI"], ymax=delay[,"MEAN"]-delay[,"CI"])) + 
  xlab("Number of handovers") + 
  ylab("Mean flow delay (s)") +
  theme_custom
dev.off()

outfile <- paste("plots/loss_rate-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(a, aes(num_handovers, loss_rate[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=loss_rate[,"MEAN"]+loss_rate[,"CI"], ymax=loss_rate[,"MEAN"]-loss_rate[,"CI"])) + 
  xlab("Number of handovers") + 
  ylab("Mean flow loss rate (%)") +
  theme_custom
dev.off()

all_dfm <- subset(all_dfm, is.finite(hop_count))

h <- with(all_dfm, aggregate(all_dfm[, c("hop_count")], list(mobility=mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)
print(h)

outfile <- paste("plots/hopcount-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(h, aes(mobility, x[,"MEAN"], fill=mobility)) +
  geom_bar(stat="identity", position="dodge") +
#  geom_errorbar(aes(ymin=x[,"MEAN"]+x[,"CI"], ymax=x[,"MEAN"]-x[,"CI"])) +
  xlab("Mobility scheme") +
  ylab("Average path length") +
  theme_custom
dev.off()

h <- with(all_dfm, aggregate(all_dfm[, c("hop_count")], list(num_handovers=num_handovers,mobility=mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)
print(h)

# FAIL
outfile <- paste("plots/hopcount-handover-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(h, aes(num_handovers, x[,"MEAN"], fill=mobility)) +
  geom_bar(stat="identity", position="dodge") +
#  geom_errorbar(aes(ymin=x[,"MEAN"]+x[,"CI"], ymax=x[,"MEAN"]-x[,"CI"])) +
  xlab("Mobility scheme") +
  ylab("Average path length") +
  theme_custom
dev.off()

