tests = {
    'task': [common_dep],
}

foreach name, deps : tests
    exe = executable(
        name + '_test',
        name + '_test.cpp',
        dependencies: deps + [gtest_dep]
    )
    test(name, exe, protocol: 'gtest')
endforeach
