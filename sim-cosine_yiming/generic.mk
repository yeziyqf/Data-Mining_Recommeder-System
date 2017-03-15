#
# Generic Makefile
#

RM = rm -f

all: $(BIN) clean_objs

%.o: $(SOURCE)/%.c
	$(CXX) $(WDEBUG) $(CCFLAGS)  $(CPPFLAGS) -c $< -o $@

%.o: $(SOURCE)/%.cpp
	$(CXX) $(WDEBUG) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(WDEBUG) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean_objs:
	@$(RM) $(OBJS)

clean: clean_objs
	$(RM) $(BIN)
