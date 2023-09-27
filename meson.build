project('many21','c')

add_global_arguments('-O2', language : 'c')

src_files = ['main.c', 'workq.c', 'emq.c','emq1.c','emq2.c','emq3.c','emq4.c']

thread_dep = dependency('threads')

executable('many21', sources : src_files ,dependencies : thread_dep)
