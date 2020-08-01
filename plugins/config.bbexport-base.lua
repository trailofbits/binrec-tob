s2e = {
    kleeArgs = {
        -- use concolic mode and disable preemptive forking for performance
        "--use-concolic-execution",
        "--enable-speculative-forking=false",

        "--keep-llvm-functions",

        -- don't output final.bc or op_helpers.ll
        "--output-module=false", "--output-source=false",

        "--randomize-fork",
        "--use-non-uniform-random-search",
        "--use-batching-search", --"--batch-time=1.0",
        "--state-shared-memory=true",
    }
}

plugins = {
    'BaseInstructions',
    'RawMonitor',
    'ModuleExecutionDetector',
    'HostFiles',
    'ELFSelector',
    'BBExport',
}

pluginsConfig = {}

pluginsConfig.RawMonitor = {
    kernelStart = 0xc0000000
}

pluginsConfig.ModuleExecutionDetector = {
    trackAllModules = false,
    configureAllModules = false
}

pluginsConfig.HostFiles = {
    baseDirs = {
        'qemu',
        'test',
        'test/dhrystone-2.1',
        'test/coreutils-8.23',
        'test/aiethel',
        'test/spec2006/benchspec/CPU2006/401.bzip2/exe',
        'test/spec2006/benchspec/CPU2006/401.bzip2/data/ref/input',
        'test/spec2006/benchspec/CPU2006/401.bzip2/data/all/input',
        'test/spec2006/benchspec/CPU2006/429.mcf/exe',
        'test/spec2006/benchspec/CPU2006/429.mcf/data/ref/input',
        'test/spec2006/benchspec/CPU2006/456.hmmer/exe',
        'test/spec2006/benchspec/CPU2006/456.hmmer/data/ref/input',
        'test/spec2006/benchspec/CPU2006/458.sjeng/exe',
        'test/spec2006/benchspec/CPU2006/458.sjeng/data/ref/input',
        'test/spec2006/benchspec/CPU2006/462.libquantum/exe',
        'test/spec2006/benchspec/CPU2006/462.libquantum/data/ref/input',
        'test/spec2006/benchspec/CPU2006/464.h264ref/exe',
        'test/spec2006/benchspec/CPU2006/464.h264ref/data/ref/input',
        'test/spec2006/benchspec/CPU2006/464.h264ref/data/all/input',
        'test/spec2006/benchspec/CPU2006/473.astar/exe',
        'test/spec2006/benchspec/CPU2006/473.astar/data/ref/input',
        'test/spec2006/benchspec/CPU2006/445.gobmk/exe',
        'test/spec2006/benchspec/CPU2006/445.gobmk/data/ref/input',
        'test/spec2006/benchspec/CPU2006/400.perlbench/exe',
        'test/spec2006/benchspec/CPU2006/471.omnetpp/exe',
        'test/spec2006/benchspec/CPU2006/403.gcc/exe',
        'test/spec2006/benchspec/CPU2006/483.xalancbmk/exe',
    }
}

pluginsConfig.BBExport = { predecessor = '', address = '', prevSuccs = '', symbols = ''}

