#!/usr/bin/env Rscript

library(ggplot2)
library(RSvgDevice)

source("scripts/theme_custom.R")

input=commandArgs(TRUE)

filenames <- list.files(path=input, pattern="load")
for (file in filenames) {
    fn <- paste(input, file, sep="")
    cat(fn);
    temp_dfm <- read.table(fn) # , header=TRUE);
    temp_dfm$alpha <- factor(gsub(".*-a([^-]*)-.*", "\\1", fn))
    if (!exists("dfm")){
        dfm <- temp_dfm
    } else {
        dfm<-rbind(dfm, temp_dfm)
    }
    rm(temp_dfm)
}

summary(dfm)

g <- ggplot(dfm, aes(V1, V2, group=alpha, color = alpha)) + 
        geom_line() + 
        geom_hline(yintercept=0.25) +
        xlab("Time (s)") +
        ylab("Load") +
        #xlim(0, 50000) +
        #ylim(0, 1) +
        theme_custom

# XXX output ??
devSVG("load.svg")
print(g)
dev.off()


