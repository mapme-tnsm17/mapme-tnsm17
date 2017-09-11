#!/usr/bin/env Rscript

library(ggplot2)

source('scripts/theme_custom.R')

filenames <- Sys.glob("data/r0.5-*/1/flow*")

if (exists("dfm")){
  rm(dfm)
}
for (file in filenames) {
    temp_dfm <- read.table(file, header=TRUE);
    temp_dfm$down <- grepl("-down-", file);
    temp_dfm$mean_sleep <- as.numeric(gsub(".*-i([^-]*?)-.*", "\\1", file))
    temp_dfm$mobility   <- gsub(".*-m([^/]*?)/.*", "\\1", file)
      
    if (!exists("dfm")){
        dfm <- temp_dfm
    } else {
        dfm<-rbind(dfm, temp_dfm)
    }
    rm(temp_dfm)
}
dfm$mobility <- as.factor(dfm$mobility)

a <- with(dfm, aggregate(dfm[, c("goodput", "elapsed", "num_holes", "rate")], list(mean_sleep=mean_sleep, mobility=mobility), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

svg("plots/goodput_vs_mean_sleep-r0.25.svg")
ggplot(a, aes(mean_sleep, goodput[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=goodput[,"MEAN"]+goodput[,"CI"], ymax=goodput[,"MEAN"]-goodput[,"CI"])) + 
  scale_x_log10() +
  xlab("Mean think time at base station (s)") +
  ylab("Mean flow goodput (Mb/s)") +
  theme_custom_tl
dev.off()

svg("plots/rate_vs_mean_sleep-r0.25.svg")
ggplot(a, aes(mean_sleep, rate[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=rate[,"MEAN"]+rate[,"CI"], ymax=rate[,"MEAN"]-rate[,"CI"])) + 
  scale_x_log10() +
  xlab("Mean think time at base station (s)") +
  ylab("Mean flow rate (Mb/s)") +
  theme_custom_tl
dev.off()

svg("plots/elapsed_vs_mean_sleep-r0.25.svg")
ggplot(a, aes(mean_sleep, elapsed[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=elapsed[,"MEAN"]+elapsed[,"CI"], ymax=elapsed[,"MEAN"]-elapsed[,"CI"])) + 
  scale_x_log10() +
  xlab("Mean think time at base station (s)") +
  ylab("Mean flow elapsed (Mb/s)") +
  theme_custom
dev.off()

svg("plots/num_holes_vs_mean_sleep-r0.25.svg")
ggplot(a, aes(mean_sleep, num_holes[,"MEAN"], group=mobility, color=mobility)) +
  geom_line() +
  geom_errorbar(aes(ymin=num_holes[,"MEAN"]+num_holes[,"CI"], ymax=num_holes[,"MEAN"]-num_holes[,"CI"])) + 
  scale_x_log10() +
  xlab("Mean think time at base station (s)") +
  ylab("Mean flow num_holes (Mb/s)") +
  theme_custom
dev.off()

a <- aggregate(dfm$elapsed, list(dfm$mean_sleep, dfm$mobility), mean)
svg("plots/elapsed_vs_mean_sleep-r0.25.svg")
ggplot(a, aes(Group.1, x, group=Group.2, color=Group.2)) +
  geom_line() +
  scale_x_log10() +
  ylim(0,7) +
  xlab("Mean think time at base station (s)") +
  ylab("Mean flow completion time (s)") +
  theme_custom
dev.off()

a <- aggregate(dfm$num_holes, list(dfm$mean_sleep, dfm$mobility), mean)
svg("plots/num_holes_vs_mean_sleep-r0.25.svg")
ggplot(a, aes(Group.1, x, group=Group.2, color=Group.2)) +
  geom_line() +
  scale_x_log10() +
  ylim(0, 45) +
  xlab("Mean think time at base station (s)") +
  ylab("Mean number of holes per flow") +
  theme_custom
dev.off()

a <- aggregate(dfm$rate, list(dfm$mean_sleep, dfm$mobility), mean)
svg("plots/rate_vs_mean_sleep-r0.25.svg")
ggplot(a, aes(Group.1, x, group=Group.2, color=Group.2)) +
  geom_line() +
  scale_x_log10() +
  ylim(0, 1.5) +
  xlab("Mean think time at base station (s)") +
  ylab("Mean flow rate (Mb/s)") +
  theme_custom
dev.off()
