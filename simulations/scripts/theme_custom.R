require(grid)

theme_custom <- theme_bw() + 
    theme(legend.justification=c(1,1),
        legend.position=c(1,1),
        legend.margin=unit(2,"lines"),
        legend.box="vertical",
        legend.key.size=unit(2,"lines"),
        legend.text.align=0,
        legend.title.align=0,
        legend.box.just = "left") +
    theme(text = element_text(size=26))

theme_custom_tl <- theme_bw() + 
    theme(legend.justification=c(0,1),
        legend.position=c(0,1),
        legend.margin=unit(2,"lines"),
        legend.box="vertical",
        legend.key.size=unit(2,"lines"),
        legend.text.align=0,
        legend.title.align=0,
        legend.box.just = "left") +
    theme(text = element_text(size=26))

theme_custom_br <- theme_bw() + 
    theme(legend.justification=c(1,0),
        legend.position=c(1,0),
        legend.margin=unit(2,"lines"),
        legend.box="vertical",
        legend.key.size=unit(2,"lines"),
        legend.text.align=0,
        legend.title.align=0,
        legend.box.just = "left") +
    theme(text = element_text(size=26))

theme_custom_tr <- theme_bw() + 
    theme(legend.justification=c(1,1),
        legend.position=c(1,0),
        legend.margin=unit(2,"lines"),
        legend.box="vertical",
        legend.key.size=unit(2,"lines"),
        legend.text.align=0,
        legend.title.align=0,
        legend.box.just = "left") +
    theme(text = element_text(size=26))

theme_custom_out <- theme_bw() + 
    theme(legend.justification=c(0,1),
        legend.position=c(1,1),
        legend.margin=unit(2,"lines"),
        legend.box="vertical",
        legend.key.size=unit(2,"lines"),
        legend.text.align=0,
        legend.title.align=0,
        legend.box.just = "left") +
    theme(text = element_text(size=26))
