# The customizations for this particular paper
NAME=	main
VENUE=	CSE602
SRCS=	$(NAME).tex		\
	abstract.tex		\
	intro.tex		\
	bg.tex			\
	implementation.tex	\
	eval.tex		\
	related.tex		\
	conclusion.tex		\
	ack.tex

# extre dependencies
EXTRAS=../references.bib figures/*.eps

# Let the user have custom overrides, globally or for just this paper.
-include $(HOME)/.paperdev.mk
-include paperdev.mk

# Use the default Make rules
-include template/Rules.make
