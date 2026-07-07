tests = [
  'p_test',
  'v_test',
  'simple_test',
  'potato_test',
  'potato_multi_test',
]

foreach test : tests
  source = files(['@0@.c'.format(test)])
  exe = executable(
    test, source,
    include_directories : inc,
    link_with : [bss]
  )
  test('@0@'.format(test), exe, timeout: 1)
endforeach