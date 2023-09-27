project('many21','c')

add_global_arguments('-O2', language : 'c')

src_files = ['main.c', 'workq.c', 'emq.c']

thread_dep = dependency('threads')

executable('many21', sources : src_files ,dependencies : thread_dep)
