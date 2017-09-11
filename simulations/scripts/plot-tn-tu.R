#!/usr/bin/env Rscript

library(ggplot2)
library(gtable)
library(grid)
library(scales) # pretty_breaks
source('scripts/theme_custom.R')

input = commandArgs(TRUE)[1]

plot_path <- "./plots"

# LOSS RATE (duplicated code from scripts/plot-static-streaming-all.R)
# XXX We should do the same for mapme old

pattern <- paste(input, "mapme/*/flow_stats*", sep="")
filenames <- Sys.glob(pattern)

if (exists("dfm")){
  rm(dfm)
}

for (file in filenames) {
    temp_dfm <- read.table(paste(file, sep=""), header=TRUE);
    temp_dfm$Mobility <- gsub(".*-m([^/]*?)/.*", "\\1", file)
    temp_dfm$TN <- gsub(".*-tn([^-]*)-.*", "\\1", file)
    temp_dfm$TU <- gsub(".*-tu([^-]*)-.*", "\\1", file)
    if (!exists("dfm")){
        dfm <- temp_dfm
    } else {
        dfm <- rbind(dfm, temp_dfm)
    }
    rm(temp_dfm)
}

# Process fields...
dfm$loss_rate = dfm$loss_rate * 100
dfm$delay = dfm$delay * 1000
dfm$TN = as.double(dfm$TN)
dfm$TU = as.double(dfm$TU)

# ... and rename mobility factors
dfm$Mobility[dfm$Mobility == "gr"] <- "GlobalRouting"
dfm$Mobility[dfm$Mobility == "anchor"] <- "Anchor"
dfm$Mobility[dfm$Mobility == "mapme"] <- "Map-Me"
dfm$Mobility[dfm$Mobility == "mapme-iu"] <- "Map-Me-IU"
dfm$Mobility[dfm$Mobility == "mapme-old"] <- "Map-Me-Old"
dfm$Mobility <- as.factor(dfm$Mobility)

#summary(dfm);

# Mean loss rate as a function of Tu 
a <- with(dfm, aggregate(dfm[, c("delay", "loss_rate")], list(TN=TN, TU=TU), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

a$TN <- as.double(a$TN)
a$TU <- as.double(a$TU)

summary(a)

cat("--------------------------------------------------------------------------\n")

# SIGNALLING OVERHEAD (duplicated code from scripts/plot-signaling-overhead.R)

pattern <- paste(input, "mapme", sep="")
filenames <- Sys.glob(pattern)
cat("pattern");
cat(pattern);
cat("\nfilenames:");
cat(filenames);
cat("\n");

if (exists("all_ho")){
  rm(all_ho)
}

for (dir in filenames) {
  cat("=> "); cat(dir); cat("\n");

  # Loop over each scenario
  subdir_pattern <- paste(dir, "/*/", sep="")
  for (subdir in Sys.glob(subdir_pattern)) {
    # Loop over each experiment
    cat("    - subdir="); cat(subdir); cat("\n");

    fo <- read.table(paste(subdir, "forwarderOverhead.txt", sep=""))
    names(fo) <- c("num_messages")

    # Number of handover
    ho <- read.table(paste(subdir, "producerOverhead.txt", sep=""))
    if (length(names(ho)) == 3) {
      # mapme
      names(ho) <- c("time", "num_in", "num_iu")


      ho$num_messages = 0
#      if (nrow(subset(ho, num_iu == 1)) != nrow(fo)) {
#        cat("NEXT");
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
    ho$Mobility <- gsub(".*-m([^/]*?)/.*", "\\1", subdir)
    ho$TN       <- gsub(".*-tn([^-]*)-.*", "\\1", subdir)
    ho$TU       <- gsub(".*-tu([^-]*)-.*", "\\1", subdir)

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
all_ho$Mobility[all_ho$Mobility == "mapmein"] <- "Map-Me"
all_ho$Mobility[all_ho$Mobility == "mapme"] <- "Map-Me-IU"
all_ho$Mobility <- as.factor(all_ho$Mobility)
all_ho$TN <- as.double(all_ho$TN)
all_ho$TU <- as.double(all_ho$TU)

#summary(all_ho)

b <- with(all_ho, aggregate(all_ho[, c("num_messages", "num_messages")], list(TN=TN, TU=TU), function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}), simplify=FALSE)

# does not work if done in all_ho only
b$TN <- as.double(b$TN)
b$TU <- as.double(b$TU)

summary(b)



# PLOTS

# TODO : need to manually set ticks to have overlapping grids

p1 <- ggplot(b, aes(TN, num_messages[, "MEAN"])) +
    geom_line(colour="blue") +
    xlab("TN=TU") +
    ylab("Signalling overhead") +
    scale_y_continuous(lim=c(0,1.6), breaks=c(seq(0,1.6,by=.2))) +
    theme_custom +
    theme(axis.title.y = element_text(colour = "blue"))
p2 <- ggplot(a, aes(TN, loss_rate[, "MEAN"])) + 
    geom_line(colour="red") +
    ylab("Loss rate") +
    scale_y_continuous(lim=c(0,4), breaks=c(seq(0,5,by=.5))) +
    theme_custom +
    theme(axis.title.y = element_text(colour = "red")) %+replace%
    theme(panel.grid.major = element_blank(), panel.grid.minor = element_blank(), panel.background = element_rect(fill = NA))


# Experimental 
grid.newpage()

# extract gtable
g1 <- ggplot_gtable(ggplot_build(p1))
g2 <- ggplot_gtable(ggplot_build(p2))

# overlap the panel of 2nd plot on that of 1st plot
pp <- c(subset(g1$layout, name == "panel", se = t:r))
g <- gtable_add_grob(g1, g2$grobs[[which(g2$layout$name == "panel")]], pp$t, 
        pp$l, pp$b, pp$l)

# axis tweaks
ia <- which(g2$layout$name == "axis-l")
ga <- g2$grobs[[ia]]
ax <- ga$children[[2]]
ax$widths <- rev(ax$widths)
ax$grobs <- rev(ax$grobs)
ax$grobs[[1]]$x <- ax$grobs[[1]]$x - unit(1, "npc") + unit(0.15, "cm")
g <- gtable_add_cols(g, g2$widths[g2$layout[ia, ]$l], length(g$widths) - 1)
g <- gtable_add_grob(g, ax, pp$t, length(g$widths) - 1, pp$b)

outfile <- paste(plot_path, "/tn-tu-", unlist(strsplit(input, "/"))[3] ,".svg", sep="")
svg(outfile)
grid.draw(g)
dev.off()
