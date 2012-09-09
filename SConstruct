env = Environment()

# global settings/args
######################
if ARGUMENTS.get('debug', 0):
        env.Append( CCFLAGS = '-ggdb' )

env.Append( CCFLAGS = ['-Wall','-Wextra'] )
env.Append( CPPPATH= ['./inc/'] )

# agent core
############
env.StaticLibrary(      target = 'bloomfilt',
                        source = ['src/bloomfilt.c'] )

# debug agent
#############
env.Program(            target = 'bloomfilt-example',
                        source = ['src/example.c'],
                        LIBPATH= '.',
                        LIBS   = ['bloomfilt','crypto'], );
