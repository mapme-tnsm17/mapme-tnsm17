#!/usr/bin/env Rscript

input=commandArgs(TRUE)[1]

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

cat(mean(dfm$goodput))

