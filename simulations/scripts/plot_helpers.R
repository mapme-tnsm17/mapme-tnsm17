#
# Simulation and plotting helpers
#
# TODO
# - auto stringsAsFactors for dataframes
# - scenario expected as a global variable !!!!!!!!!!!!!!!!!!!!
# - computer modern font : https://github.com/wch/fontcm

# ### Load the package or install if not present
#if (!require("RColorBrewer")) {
#  install.packages("RColorBrewer")
#    library(RColorBrewer)
#}

library(ggplot2)
source('scripts/theme_custom.R')

DEFAULT_LINE_SIZE <- 1.0
DEFAULT_POINT_SIZE <- 6

# bleu rouge jaune violet vert
colors <- c("#00b0f5ff", "#f8756bff", "#a1a300ff", "#e668f3ff", "#00bd7dff")

mean_sd <- function(x) { c(MEAN=mean(x) , CI=qt(0.95/2+0.5, length(x))*(sd(x) / sqrt(length(x))) )}

get_scenario_directories <- function(scenario)
{
  command <- paste("./do status", scenario)
  tt <- read.table(text=system(command, intern=TRUE))

  # http://stackoverflow.com/questions/14205583/filtering-data-in-a-table-based-on-criteria
  #t <- subset(tt, V2 %in% c("DONE"))
  t <- tt

  return(t[,1])
}

# build_data_frame
#
# TODO: add an options defaulting to false allowing to cache resulting dataframe
# on disk, as it is done for instance in plot-rates.R
#
build_data_frame <- function(scenario, file_pattern, callback, debug) 
{
  if (missing(debug))
    debug = FALSE

  if (exists("local_dfm"))
    rm(local_dfm)

  directories <- get_scenario_directories(scenario)

  for (directory in directories) {
    if(debug)
      cat(paste("Processing directory:", directory, "\n"));

    pattern <- paste(directory, file_pattern, sep="")
    filenames <- Sys.glob(pattern)

    for (file in filenames) {
      if(debug)
        cat(paste("  . Processing file:", file, "\n"));

      # XXX parsing scenario or using bash XXX 
      temp_local_dfm <- try(read.table(paste(file, sep=""), header=TRUE))
      if (inherits(temp_local_dfm, 'try-error')) {
        cat(paste("W: Ignored empty file", file, "\n"));
        next
      }
      if (nrow(temp_local_dfm) == 0) {
        cat(paste("W: Ignored almost empty file", file, "\n"));
        next
      }
      temp_local_dfm$Mobility       <- gsub(".*-m(.*?)-T.*", "\\1", file)
      temp_local_dfm$topology       <- gsub(".*-T(.*?)-M.*", "\\1", file)
      temp_local_dfm$MobilityModel  <- gsub(".*-M(.*?)-R.*", "\\1", file)
      temp_local_dfm$RadioModel     <- gsub(".*-R([^/]*?)/.*", "\\1", file)
      temp_local_dfm$rho            <- as.double(gsub(".*-r([^-]*)-.*", "\\1", file))
      temp_local_dfm$speed          <- as.numeric(gsub(".*-e([^-]*)-.*", "\\1", file))
      temp_local_dfm$num_cp_couples <- as.numeric(gsub(".*-N([^-]*)-.*", "\\1", file))
      temp_local_dfm$run            <- as.numeric(gsub(".*/([0-9]*)/.*", "\\1", file))
      temp_local_dfm$TU             <- as.numeric(gsub(".*-tu([^-]*)-.*", "\\1", file)) / 1000
      #temp_local_dfm$down <- grepl("-down-", file);

      if (!missing(callback))
        temp_local_dfm <- callback(temp_local_dfm)

      if (!exists("local_dfm")) {
          local_dfm <- temp_local_dfm
      } else {
          local_dfm <- rbind(local_dfm, temp_local_dfm)
      }
      rm(temp_local_dfm)
    }
  }

  local_dfm$Mobility[local_dfm$Mobility == "gr"] <- "GR"
  local_dfm$Mobility[local_dfm$Mobility == "anchor"] <- "AB"
  local_dfm$Mobility[local_dfm$Mobility == "kite"] <- "TB"
  local_dfm$Mobility[local_dfm$Mobility == "mapme-iu"] <- "MapMe-IU"
  local_dfm$Mobility[local_dfm$Mobility == "mapme"] <- "MapMe"


  local_dfm$topology[local_dfm$topology == "tree"] <- "Tree"
  local_dfm$topology[local_dfm$topology == "fat-tree"] <- "FatTree"
  local_dfm$topology[local_dfm$topology == "barabasi-albert"] <- "BA"
  local_dfm$topology[local_dfm$topology == "cycle"] <- "CY"
  local_dfm$topology[local_dfm$topology == "grid2d"] <- "G2"
  local_dfm$topology[local_dfm$topology == "hypercube"] <- "HC"
  local_dfm$topology[local_dfm$topology == "erdos-renyi"] <- "ER"
  local_dfm$topology[local_dfm$topology == "expander"] <- "EX"
  local_dfm$topology[local_dfm$topology == "regular"] <- "RG"
  local_dfm$topology[local_dfm$topology == "small-world"] <- "SM"
  local_dfm$topology[local_dfm$topology == "watts-strogatz"] <- "WS"

  local_dfm$MobilityModel[local_dfm$MobilityModel == "rwp"] <- "RWP"
  local_dfm$MobilityModel[local_dfm$MobilityModel == "rw2"] <- "RW2D"
  local_dfm$MobilityModel[local_dfm$MobilityModel == "rgm"] <- "RGM"
  local_dfm$MobilityModel[local_dfm$MobilityModel == "rd2"] <- "RD2D"
  local_dfm$MobilityModel[local_dfm$MobilityModel == "markov-uniform"] <- "M-Unfm"
  local_dfm$MobilityModel[local_dfm$MobilityModel == "markov-rw"] <- "M-RW"
  local_dfm$MobilityModel[local_dfm$MobilityModel == "markov-rwp"] <- "M-RWP"

  local_dfm$RadioModel[local_dfm$RadioModel == "rfm"] <- "RFM"
  local_dfm$RadioModel[local_dfm$RadioModel == "slos"] <- "SLOS"
  local_dfm$RadioModel[local_dfm$RadioModel == "unlos"] <- "UNLOS"
  local_dfm$RadioModel[local_dfm$RadioModel == "pwc"] <- "PWC"

  # This has to be done after rename
  local_dfm$Mobility <- factor(local_dfm$Mobility, levels=c("dummy", "GR", "AB", "TB", "MapMe-IU", "MapMe"))
  local_dfm$topology <- factor(local_dfm$topology, levels=c("Tree", "FatTree", "BA", "CY", "G2", "HC", "ER", "EX", "RG", "SM", "WS"))
  local_dfm$MobilityModel <- factor(local_dfm$MobilityModel, levels=c("RWP", "RW2D", "RGM", "RD2D", "M-Unfm", "M-RW", "M-RWP"))
  local_dfm$RadioModel <- factor(local_dfm$RadioModel, levels=c("RFM", "SLOS", "UNLOS", "PWC"))

  return(local_dfm);
}

generate_plot <- function(scenario, dfm, type, errorbars, legend, param, param_name, metric, metric_name)
{
  g <- get_plot(scenario, dfm, type, errorbars, legend, param, param_name, metric, metric_name)
  make_svg(scenario, g, metric)
}

# scale_colour_manual(name="conditions", values=RColorBrewer::brewer.pal(4, "Paired"))
get_plot <- function(scenario, dfm, type, errorbars, legend, param, param_name, metric, metric_name)
{
  # XXX Nothing to do if file exists
  if (type == "line") {
    g <- ggplot(dfm, aes(dfm[,param], dfm[,metric][,"MEAN"], group=Mobility, color=Mobility, shape=Mobility)) +
      geom_line(size=DEFAULT_LINE_SIZE) +
      geom_point(size=DEFAULT_POINT_SIZE) +
      scale_shape_manual(values = c(1, 2, 3, 4, 5)) +
      scale_colour_manual(values = colors)
#      scale_colour_brewer(palette="OrRd")
  } else if (type == "bar") {
    g <- ggplot(dfm, aes(dfm[,param], dfm[,metric][,"MEAN"], fill=Mobility)) +
      geom_bar(width=0.5, stat="identity", position="dodge") +
      scale_fill_manual(values = colors)
#     scale_colour_brewer(palette="OrRd") +
# scale_fill_brewer(palette="OrRd")

  } else if (type == "cdf") {
    g <- ggplot(dfm, aes(dfm[,metric], colour=Mobility)) +
      stat_ecdf(show.legend=T, size=DEFAULT_LINE_SIZE) +
      scale_colour_manual(values = colors)
#     scale_colour_brewer(palette="OrRd")
  } else {
    cat(paste("E: Wrong graph type:", type, "\n"))
  }

  if ((errorbars == TRUE) & ((type == "line") | (type == "bar"))) {
    g <- g + geom_errorbar(aes(ymin=dfm[,metric][,"MEAN"]+dfm[,metric][,"CI"], ymax=dfm[,metric][,"MEAN"]-dfm[,metric][,"CI"]))
  }
  if ((type == "line") | (type == "bar")) {
      g <- g + xlab(param_name)
      g <- g + ylab(metric_name) + expand_limits(x = 0, y = 0)
  } else if (type == "cdf") {
      g <- g + xlab(metric_name)
      g <- g + ylab("CDF")
  }

  # Legend / theme
  if (legend == "tr") {
    g <- g + theme_custom
  } else if (legend == "tl") {
    g <- g + theme_custom_tl
  } else if (legend == "br") {
    g <- g + theme_custom_br
  }
  g <- g + theme(legend.title=element_blank())

# x label rotation
#  if (type == "bar") {
#    g <- g + theme(axis.text.x = element_text(angle = 30, hjust = 1))
#  }
  return(g)

}

make_svg <- function(scenario, g, metric)
{
  outfile <- paste("plots/", basename(scenario), "-", metric, ".svg", sep="")
  cat(paste("I: Plotting", metric, "in", outfile, "\n"));
  svg(outfile)
  print(g)
  dev.off()
}

generate_plots <- function(scenario, dfm, type, errorbars, legend, plots)
{
  for (plot in plots) {
    param <- plot[[1]]
    param_name <- plot[[2]]
    metric <- plot[[3]]
    metric_name <- plot[[4]]

    generate_plot(scenario, dfm, type, errorbars, legend, param, param_name, metric, metric_name)
  }
}
  

# XXX This is highly dependent on topology
# Can we use node labels and log them in the file instead
get_node_class <- function(node_id)
{
  if (is.na(node_id))
    return("NA");
  # MobileFatTree topology
  if (node_id <= 15) {
    return("Access")
  } else if ((node_id >= 17) & (node_id <= 20)) {
    return("Backhaul")
  } else if (node_id == 16) {
    return("Core")
  } else if ((node_id >= 21) & (node_id <= 28)) {
    return("Edge");
  } else {
    return("Mobile")
  }

  # MobileFatTree with Border AP
  #if (node_id <= 35) {
  #  return("Access")
  #} else if ((node_id >= 37) & (node_id <= 40)) {
  #  return("Backhaul")
  #} else if (node_id == 36) {
  #  return("Core")
  #} else if ((node_id >= 41) & (node_id <= 52)) {
  #  return("Edge");
  #} else {
  #  return("Mobile")
  #}
}

get_link_id <- function(src_node_id, dst_node_id)
{
  return(paste(min(src_node_id, dst_node_id), max(src_node_id, dst_node_id), sep="-"))
}

get_link_class <- function(src_node_id, dst_node_id)
{
  src_node_class <- get_node_class(src_node_id)
  dst_node_class <- get_node_class(dst_node_id)

  if (src_node_class == "Core") {
    # Valid: Backhaul->Core=Core
    if (dst_node_class == "Backhaul") {
      return("Core"); #CoreBackhaul");
    }
    
  } else if (src_node_class == "Backhaul") {
    # Valid: Backhaul->Core=Core; Mesh Backhaul; Access->Backhaul=backhaul
    if (dst_node_class == "Core") {
      return("Core"); #"CoreBackhaul");
    } else if (dst_node_class == "Backhaul") {
      return("Backhaul"); # MESH BACKHAUL
    } else if (dst_node_class == "Edge") {
      return("Backhaul"); 
    }
    
  } else if (src_node_class == "Edge") {
    # Valid: Edge->Backhaul=Backhaul; Mesh Edge; Access->Edge=Edge
    if (dst_node_class == "Backhaul") {
      return("Backhaul"); #BackhaulEdge");
    } else if (dst_node_class == "Edge") {
      return("Edge"); # MESH EDGE
    } else if (dst_node_class == "Access") {
      return("Edge"); #EdgeAccess");
    }

  } else if (src_node_class == "Access") {
    # Valid: Mobile->Access=Access; Mesh Access; Access->Edge=Edge
    if (dst_node_class == "Edge") {
      return("Edge"); #AccessEdge");
    } else if (dst_node_class == "Access") {
      return("Access"); # MESH ACCESS
    } else {
      return("Access"); #MobileAccess");
    }

  } else { # Mobile
    if (dst_node_class == "Access") {
      return("Access"); #MobileAccess");
    }

  }
}

# Core > Backhaul > Edge > Access > Mobile
# NOTE: not directional, mainly used for X2 links instrumentation
get_link_class_ext <- function(src_node_id, dst_node_id)
{
  src_node_class <- get_node_class(src_node_id)
  dst_node_class <- get_node_class(dst_node_id)

  if (src_node_class == "Core") {
    # Valid: Backhaul->Core=Core
    if (dst_node_class == "Backhaul") {
      return("CoreBackhaul");
    }
    
  } else if (src_node_class == "Backhaul") {
    # Valid: Backhaul->Core; Mesh Backhaul; Access->Backhaul
    if (dst_node_class == "Core") {
      return("CoreBackhaul");
    } else if (dst_node_class == "Backhaul") {
      return("MeshBackhaul");
    } else if (dst_node_class == "Edge") {
      return("BackhaulEdge"); 
    }
    
  } else if (src_node_class == "Edge") {
    # Valid: Edge->Backhaul; Mesh Edge; Access->Edge
    if (dst_node_class == "Backhaul") {
      return("BackhaulEdge");
    } else if (dst_node_class == "Edge") {
      return("MeshEdge");
    } else if (dst_node_class == "Access") {
      return("EdgeAccess"); 
    }

  } else if (src_node_class == "Access") {
    # Valid: Mobile->Access; Mesh Access; Access->Edge
    if (dst_node_class == "Edge") {
      return("EdgeAccess");
    } else if (dst_node_class == "Access") {
      return("MeshAccess");
    } else {
      return("AccessMobile");
    }

  } else { 
    # Valid: Access->Mobile
    if (dst_node_class == "Access") {
      return("AccessMobile");
    }

  }
}

