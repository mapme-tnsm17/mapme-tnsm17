#!/usr/bin/env Rscript

library(ggplot2)
library(RSvgDevice)

source("scripts/theme_custom.R")

alpha = c(0.8, 1.2)
catalog_size = c(1000, 10000)

for (a in alpha) {
    for (k in catalog_size) {
        fn <- paste("data/r0.5-K", k, "-C1-s100-a", a, "-d100000/1/popularity-c1", sep="")
        temp_dfm <- read.table(fn); #, header=TRUE);
        temp_dfm$alpha <- a
        temp_dfm$catalog_size <- k
        if (!exists("dfm")){
            dfm <- temp_dfm
        } else {
            dfm<-rbind(dfm, temp_dfm)
        }
        rm(temp_dfm)
    }
}
dfm$alpha <- as.factor(dfm$alpha)
dfm$catalog_size <- as.factor(dfm$catalog_size)

summary(dfm)

g <- ggplot(dfm, aes(V1, V6, group=interaction(alpha, catalog_size), color = alpha, size = catalog_size)) + 
        geom_line() + 
        #scale_size(to = c(0 * 10, 3 * 10)) +
        xlab("Content rank") +
        ylab("Hitrate") +
        scale_x_log10(limits=c(1,100)) +
        scale_size_manual(values=c(0.7, 1.2)) + 
        theme_custom
devSVG("plots/hitrate.svg")
print(g)
dev.off()
