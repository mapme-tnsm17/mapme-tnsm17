#!/usr/bin/env Rscript

library(ggplot2)
source('scripts/theme_custom.R')

input=commandArgs(TRUE)[1]
plot_path <- "./plots"

pattern <- paste(input, "*", sep="")
filenames <- Sys.glob(pattern)

if (exists("all_ho")){
  rm(all_ho)
}

for (dir in filenames) {
  if (grepl("-mgr", dir))
      next
  cat(dir)
  cat("\n")
  # Loop over each scenario
  subdir_pattern <- paste(dir, "/*/", sep="")
  cat("SUBDIR PATTERN: ")
  cat(subdir_pattern)
  cat("\n")
  for (subdir in Sys.glob(subdir_pattern)) {
    # Loop over each experiment
    cat("SUBDIR : ")
    cat(subdir)
    cat("\n")

    fo <- read.table(paste(subdir, "forwarderOverhead.txt", sep=""))
    names(fo) <- c("num_messages")

    # Number of handover
    ho <- read.table(paste(subdir, "producerOverhead.txt", sep=""))
    if (length(names(ho)) == 3) {
      # mapme
      names(ho) <- c("time", "num_in", "num_iu")


      ho$num_messages = 0

# XXX WHY THIS TEST ?
#      if (nrow(subset(ho, num_iu == 1)) != nrow(fo)) {
#        cat("NEXT\n") # XXX I have a problem here !!
#        next
#      }
      ho$num_messages[ho$num_iu == 1] = fo$num_messages - 1 # Otherwise, IU cost


      drops <- c("num_in", "num_iu")
      ho <- ho[,!(names(ho) %in% drops)]     
    } else { # 2
      # anchor
      names(ho) <- c("time", "num_messages")


      ho$num_messages <- ho$num_messages * 1:nrow(ho) * fo$num_messages
    }
    ho$num_handover = 1:nrow(ho)
    cat("MOB   ")
    cat(subdir)
    cat("\n")
    ho$Mobility   <- gsub(".*-m([^/]*?)/.*", "\\1", subdir)

    if (!exists("all_ho")){
        all_ho <- ho
    } else {
        all_ho<-rbind(all_ho, ho)
    }
    rm(ho)


  }
}

all_ho$Mobility[all_ho$Mobility == "gr"] <- "GlobalRouting"
all_ho$Mobility[all_ho$Mobility == "anchor"] <- "Anchor"
all_ho$Mobility[all_ho$Mobility == "mapme"] <- "Map-Me"
all_ho$Mobility[all_ho$Mobility == "mapme-iu"] <- "Map-Me-IU"
all_ho$Mobility[all_ho$Mobility == "mapme-old"] <- "Map-Me-Old"

#all_ho$Mobility <- as.factor(all_ho$Mobility)

a <- with(all_ho, aggregate(all_ho[, c("num_messages")], list(num_handover=num_handover, Mobility=Mobility), mean)) #function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

#for (i in seq(nrow(a),nrow(a)+21)) {
#  a[i, ] <- c(i, 13, i-21, "GlobalRouting")
#}
a <- subset(a, num_handover < 16)

outfile <- paste(plot_path, "/signaling-overhead-", unlist(strsplit(input, "/"))[3] ,".svg", sep="")
svg(outfile)
ggplot(a, aes(num_handover, x, group=Mobility, color=Mobility)) +
  geom_line() +
  geom_point() +
  xlab("Number of handovers") + 
  ylab("Average number of routers updated") + 
  scale_y_continuous(breaks = seq(0, 15, by=1), lim=c(0,15)) +
  theme_custom
dev.off()
