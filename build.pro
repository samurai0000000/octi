# build.pro
#
# Copyright (C) 2022, Charles Chiou

TEMPLATE = subdirs
SUBDIRS += ai/ai.pro game/game.pro util/util.pro widget/widget.pro octi.pro

octi.depends = ai/ai.pro game/game.pro util/util.pro widget/widget.pro
