#!/usr/bin/env Rscript

# Plot streaming flow statistics
# Sample usage (yes, truncate path after -m parameter)
# ./scripts/plotall.R ./data/r0.7-K1000-C0-s3000-a0.8-d1000-e10-p0-b20-c50-m

library(ggplot2)
source('scripts/theme_custom.R')

input=commandArgs(TRUE)[1]
plot_path <- "plots"

if (exists("all_dfm")){
  rm(all_dfm)
}

pattern <- paste(input, "*/*/goodput*", sep="")
cat(pattern)
  cat("\n");
filenames <- Sys.glob(pattern)

if (exists("dfm")){
  rm(dfm)
}

for (file in filenames) {
    temp_dfm <- read.table(paste(file, sep=""), header=TRUE);
    temp_dfm$down <- grepl("-down-", file);
    temp_dfm$Mobility   <- gsub(".*-m([^/]*?)/.*", "\\1", file)
    if (!exists("dfm")){
        dfm <- temp_dfm
    } else {
        dfm<-rbind(dfm, temp_dfm)
    }
    rm(temp_dfm)
}

dfm$Mobility[dfm$Mobility == "gr"] <- "GlobalRouting"
dfm$Mobility[dfm$Mobility == "anchor"] <- "Anchor"
dfm$Mobility[dfm$Mobility == "mapme"] <- "Map-Me"
dfm$Mobility[dfm$Mobility == "mapme-iu"] <- "Map-Me-IU"
dfm$Mobility[dfm$Mobility == "mapme-old"] <- "Map-Me-Old"

summary(dfm);

a <- with(dfm, aggregate(dfm[, c("goodput", "hop_count")], list(num_handover=num_handover, Mobility=Mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

outfile <- paste("plots/goodput-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(a, aes(num_handover, goodput[,"MEAN"], group=Mobility, color=Mobility)) +
  geom_line() +
#  geom_errorbar(aes(ymin=goodput[,"MEAN"]+goodput[,"CI"], ymax=goodput[,"MEAN"]-goodput[,"CI"])) + 
  xlab("Number of handovers") + 
  ylab("Goodput (Mb/s)") +
  xlim(0, 30) +
  ylim(0, 10) +
  theme_custom_br
dev.off()

dfm <- subset(dfm, is.finite(hop_count))

h <- with(dfm, aggregate(dfm[, c("hop_count")], list(Mobility=Mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)
print(h)

outfile <- paste("plots/hopcount-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(h, aes(Mobility, x[,"MEAN"], fill=Mobility)) +
  geom_bar(stat="identity", position="dodge") +
#  geom_errorbar(aes(ymin=x[,"MEAN"]+x[,"CI"], ymax=x[,"MEAN"]-x[,"CI"])) +
  xlab("Mobility scheme") +
  ylab("Average path length") +
  theme_custom
dev.off()

h <- with(dfm, aggregate(dfm[, c("hop_count")], list(num_handover=num_handover,Mobility=Mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

hh <- subset(h, num_handover < 16)

outfile <- paste("plots/hopcount-handover-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(hh, aes(num_handover, x[,"MEAN"], fill=Mobility)) +
  geom_bar(stat="identity", position="dodge") +
#  geom_errorbar(aes(ymin=x[,"MEAN"]+x[,"CI"], ymax=x[,"MEAN"]-x[,"CI"])) +
  xlab("Number of handovers") +
  ylab("Average path length") +
  xlim(0, 16) +
  theme_custom
dev.off()

