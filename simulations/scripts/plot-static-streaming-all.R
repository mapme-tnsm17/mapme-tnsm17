#!/usr/bin/env Rscript

# Plot streaming flow statistics
# Sample usage (yes, truncate path after -m parameter)
# ./scripts/plotall.R ./data/r0.7-K1000-C0-s3000-a0.8-d1000-e10-p0-b20-c50-m

library(ggplot2)
source('scripts/theme_custom.R')

input=commandArgs(TRUE)[1]
plot_path <- "plots"

pattern <- paste(input, "*/*/flow_stats_streaming*", sep="")
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

dfm$loss_rate = dfm$loss_rate * 100
dfm$delay = dfm$delay * 1000
summary(dfm);

dfm$Mobility[dfm$Mobility == "gr"] <- "GlobalRouting"
dfm$Mobility[dfm$Mobility == "anchor"] <- "Anchor"
dfm$Mobility[dfm$Mobility == "mapme"] <- "Map-Me"
dfm$Mobility[dfm$Mobility == "mapme-iu"] <- "Map-Me-IU"
dfm$Mobility[dfm$Mobility == "mapme-old"] <- "Map-Me-Old"

a <- with(dfm, aggregate(dfm[, c("elapsed", "delay", "loss_rate", "hop_count")], list(num_handover=num_handover, Mobility=Mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)
a$Mobility <- as.factor(a$Mobility)
#summary(a)
#print(a$elapsed[,"MEAN"])


#outfile <- paste(plot_path, "/elapsed-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
#svg(outfile)
#ggplot(a, aes(num_handover, elapsed[,"MEAN"], group=Mobility, color=Mobility)) +
#  geom_line() +
#  geom_point() +
##  geom_errorbar(aes(ymin=elapsed[,"MEAN"]+elapsed[,"CI"], ymax=elapsed[,"MEAN"]-elapsed[,"CI"])) + 
#  xlab("Number of handovers") + 
#  ylab("Mean flow completion time (s)") +
#  theme_custom
#dev.off()

outfile <- paste(plot_path, "/delay-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(a, aes(num_handover, delay[,"MEAN"], group=Mobility, color=Mobility)) +
  geom_line() +
  geom_point() +
#  geom_errorbar(aes(ymin=delay[,"MEAN"]+delay[,"CI"], ymax=delay[,"MEAN"]-delay[,"CI"])) +
  xlab("Number of handovers") + 
  ylab("Average delay (ms)") + 
  ylim(40,80) +
  theme_custom
dev.off()

outfile <- paste(plot_path, "/loss-rate-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(a, aes(num_handover, loss_rate[,"MEAN"], group=Mobility, shape=Mobility, color=Mobility)) +
  geom_line() +
  geom_point() +
#  geom_errorbar(aes(ymin=loss_rate[,"MEAN"]+loss_rate[,"CI"], ymax=loss_rate[,"MEAN"]-loss_rate[,"CI"])) +
  xlab("Number of handovers") + 
  ylab("Average loss rate (%)") +
  scale_x_continuous(breaks = round(seq(min(a$num_handover), max(a$num_handover), by = 1),1), lim=c(0,15)) +
  scale_y_continuous(breaks = seq(0, 11, by=1), lim=c(0,10)) +
  theme_custom_tl
dev.off()

dfm <- subset(dfm, is.finite(hop_count))

#h <- with(dfm, aggregate(dfm[, c("hop_count")], list(mobility=mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)
#print(h)
#
#outfile <- paste(plot_path, "/hopcount-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
#svg(outfile)
#ggplot(h, aes(mobility, x[,"MEAN"], fill=mobility)) +
#  geom_bar(stat="identity", position="dodge") +
##  geom_errorbar(aes(ymin=x[,"MEAN"]+x[,"CI"], ymax=x[,"MEAN"]-x[,"CI"])) +
#  xlab("Mobility scheme") +
#  ylab("Average path length") +
#  theme_custom
#dev.off()

h <- with(dfm, aggregate(dfm[, c("hop_count")], list(num_handover=num_handover,Mobility=Mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)
print(h)

hh <- subset(h, num_handover < 16)
outfile <- paste(plot_path, "/hopcount-handover-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(hh, aes(num_handover, x[,"MEAN"], fill=Mobility)) +
  geom_bar(stat="identity", position="dodge") +
  xlab("Number of handovers") + 
  ylab("Average path length") +
  scale_x_continuous(breaks = round(seq(min(hh$num_handover), max(hh$num_handover), by = 1),1), lim=c(0,15)) +
  scale_y_continuous(breaks = seq(0, 11, by=1)) +
  theme_custom_br
dev.off()


###########################################

# Mean number of message per handover per solution
pattern <- paste(input, "*/*/forwarderOverhead*", sep="")
cat(pattern)
  cat("\n");
filenames <- Sys.glob(pattern)

if (exists("dfm")){
  rm(dfm)
}

for (file in filenames) {
    temp_dfm <- read.table(paste(file, sep=""));
    names(temp_dfm) <- c("num_routers")
    temp_dfm$Mobility   <- gsub(".*-m([^/]*?)/.*", "\\1", file)
    if (!exists("dfm")){
        dfm <- temp_dfm
    } else {
        dfm<-rbind(dfm, temp_dfm)
    }
    rm(temp_dfm)
}

dfm$Mobility <- as.factor(dfm$Mobility)


a <- with(dfm, aggregate(dfm[, c("num_routers")], list(Mobility=Mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

outfile <- paste(plot_path, "/signaling-overhead-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
svg(outfile)
ggplot(a, aes(Mobility, x[,"MEAN"], fill=Mobility)) +
  geom_bar(stat="identity", position="dodge") +
#  geom_errorbar(aes(ymin=x[,"MEAN"]+x[,"CI"], ymax=x[,"MEAN"]-x[,"CI"])) +
#  xlab("Mobility scheme & Mean number of handovers") +
#  ylab("Average path length") +
  theme_custom
dev.off()

# USELESS # ###########################################
# USELESS # 
# USELESS # pattern <- paste(input, "*/*/producerOverhead*", sep="")
# USELESS # cat(pattern)
# USELESS #   cat("\n");
# USELESS # filenames <- Sys.glob(pattern)
# USELESS # 
# USELESS # if (exists("dfm")){
# USELESS #   rm(dfm)
# USELESS # }
# USELESS # 
# USELESS # for (file in filenames) {
# USELESS #     cat(file)
# USELESS #     cat("\n");
# USELESS #     temp_dfm <- read.table(paste(file, sep=""));
# USELESS #     cat(length(names(temp_dfm)))
# USELESS #     cat(names(temp_dfm))
# USELESS #     cat("\n");
# USELESS #     if (length(names(temp_dfm)) == 3) {
# USELESS #       # mapme
# USELESS #       names(temp_dfm) <- c("time", "num_in", "num_iu")
# USELESS #       temp_dfm$num_messages <- temp_dfm$num_in + temp_dfm$num_iu
# USELESS #       drops <- c("num_in", "num_iu")
# USELESS #       temp_dfm <- temp_dfm[,!(names(temp_dfm) %in% drops)]
# USELESS #     } else { # 2
# USELESS #       # anchor
# USELESS #       names(temp_dfm) <- c("time", "num_messages")
# USELESS #     }
# USELESS #     temp_dfm$mobility   <- gsub(".*-m([^/]*?)/.*", "\\1", file)
# USELESS #     if (!exists("dfm")){
# USELESS #         dfm <- temp_dfm
# USELESS #     } else {
# USELESS #         dfm<-rbind(dfm, temp_dfm)
# USELESS #     }
# USELESS #     rm(temp_dfm)
# USELESS # }
# USELESS # 
# USELESS # dfm$mobility <- as.factor(dfm$mobility)
# USELESS # 
# USELESS # outfile <- paste(plot_path, "/signaling-overhead-2-", unlist(strsplit(input, "/"))[3], ".svg", sep="")
# USELESS # svg(outfile)
# USELESS # ggplot(dfm, aes(mobility, num_messages, fill=mobility)) +
# USELESS #   geom_bar(stat="identity", position="dodge") +
# USELESS # #  geom_errorbar(aes(ymin=x[,"MEAN"]+x[,"CI"], ymax=x[,"MEAN"]-x[,"CI"])) +
# USELESS # #  xlab("Mobility scheme & Mean number of handovers") +
# USELESS # #  ylab("Average path length") +
# USELESS #   theme_custom
# USELESS # dev.off()
