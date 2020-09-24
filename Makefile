# R Package Name & Version
R_PKGNAME = OpenABMCovid19
VERSION		= 0.3

ifeq ($(OS),Windows_NT)
	R_BIN_PKG_EXT=zip
else
	R_BIN_PKG_EXT=tgz
endif



# Temporary directory:
#
# Files generated using the '>' stdout redirection (e.g. sed -e STUFF.. > out)
# always update the timestamp of the output file, even when the command failed
# (e.g. because of a syntax error). This breaks make's dependency checking. As
# a workaround, we can output files to this temporary dir and move them if the
# operation was successful.
#
# We also also this directory for storing intermediate files created by R CMD.
TMPDIR = $(R_PKGNAME).tmp
ALL_OUTPUT = $(TMPDIR)



# SWIG generated files
SWIG_SRC  = src/covid19.i src/model_utils.i src/params_utils.i
SWIG_COUT = src/covid19_wrap_R.c
SWIG_ROUT = R/$(R_PKGNAME).R

$(SWIG_COUT): $(SWIG_SRC)
	swig -r -package $(R_PKGNAME) -Isrc -o $(SWIG_COUT) -outdir R src/covid19.i
$(SWIG_ROUT): $(SWIG_COUT)
	touch $(SWIG_ROUT)

ALL_OUTPUT += $(SWIG_COUT) $(SWIG_ROUT)



# R package content
R_SRC= $(SWIG_ROUT)
C_SRC= $(SWIG_COUT) src/constant.c src/constant.h src/demographics.c \
	src/demographics.h src/disease.c src/disease.h src/doctor.c src/doctor.h \
	src/hospital.c src/hospital.h src/individual.c src/individual.h src/input.c \
	src/input.h src/interventions.c src/interventions.h src/list.c src/list.h \
	src/main.c src/model.c src/model.h src/network.c src/network.h src/nurse.c \
	src/nurse.h src/params.c src/params.h src/structure.h src/utilities.c \
	src/utilities.h src/ward.c src/ward.h
CONTENT = NAMESPACE DESCRIPTION LICENSE $(R_SRC) $(C_SRC)


# R package content rules:
NAMESPACE: $(SWIG_ROUT)
	[ -d $(TMPDIR) ] || mkdir -p $(TMPDIR)
	grep '`.*` = function(.*)' $(SWIG_ROUT) \
	  | sed 's|`\(.*\)`.*|export( \1 )|' \
	  > $(TMPDIR)/NAMESPACE
	[ $$? -eq 0 ] && cp $(TMPDIR)/NAMESPACE NAMESPACE
ALL_OUTPUT += NAMESPACE

DESCRIPTION: DESCRIPTION.in .git/HEAD
	[ -d $(TMPDIR) ] || mkdir -p $(TMPDIR)
	sed \
	  -e "s/@R_PKGNAME@/$(R_PKGNAME)/" \
	  -e "s/@VERSION@/$(VERSION)/" \
	  -e "s/@GIT_TAG@/$$(git describe --tags)/" \
	  -e "s/@DATE@/$$(date +%Y-%m-%d)/" \
	  DESCRIPTION.in > $(TMPDIR)/DESCRIPTION
	[ $$? -eq 0 ] && cp $(TMPDIR)/DESCRIPTION DESCRIPTION
ALL_OUTPUT += DESCRIPTION



# Build R source package
R_SRC_PKG = $(R_PKGNAME)_$(VERSION).tar.gz
$(R_SRC_PKG): .Rbuildignore $(CONTENT)
	R CMD build .
ALL_OUTPUT += $(R_SRC_PKG)



# Build R binary package
R_BIN_PKG=$(R_PKGNAME)_$(VERSION).$(R_BIN_PKG_EXT)
$(R_BIN_PKG): $(R_SRC_PKG)
	[ -d $(TMPDIR)/Rinstall ] || mkdir -p $(TMPDIR)/Rinstall
	R CMD INSTALL --library=$(TMPDIR)/Rinstall --build $(R_SRC_PKG)
ALL_OUTPUT += $(R_BIN_PKG)



# Alias target (Phony) for convenience
Rbuild: $(R_SRC_PKG)

Rinstall: $(R_BIN_PKG)

Rcheck: $(TMPDIR_EXISTS) $(R_SRC_PKG)
	[ -d $(TMPDIR)/Rcheck ] || mkdir -p $(TMPDIR)/Rcheck
	R CMD check --output=$(TMPDIR)/Rcheck $(R_SRC_PKG)



# Cleaning
clean:
	rm -fr $(ALL_OUTPUT)

dry_clean:
	@echo '`make clean` will run:'
	@echo rm -fr $(ALL_OUTPUT)

.PHONY: clean dry_clean Rbuild Rcheck Rinstall