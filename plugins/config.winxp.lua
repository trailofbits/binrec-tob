s2e = {
    kleeArgs = {
        -- complete each state before switching to the next
        "--use-dfs-search=true",

        -- use concolic mode and disable preemptive forking for performance
        "--use-concolic-execution=true",
        "--enable-speculative-forking=false",

        -- output final.bc instead of op_helpers.ll
        "--output-module=true", "--output-source=false",

        -- don't truncate source lines in op_helpers.ll
        --"--output-source=true", "--no-truncate-source-lines=true",

        -- this is needed to make forceCodeGen work
        "--keep-llvm-functions=true",
    }
}

plugins = {
    'BaseInstructions',
    'WindowsMonitor',
    'ModuleExecutionDetector',
    'PESelector',
    'PELibCallMonitor',
    'SymbolizeArgsPE',
    'ExportPE',
    --'AddrLog',
    --'RegisterLog',
    --'DebugLog',

    -- needed for coverage tool
    'ExecutionTracer',
    'TranslationBlockTracer',
    'ModuleTracer',
}

pluginsConfig = {}

pluginsConfig.WindowsMonitor = {
    version = 'XPSP3',
    userMode = true,
    kernelMode = false,
    monitorModuleLoad = true,
    monitorModuleUnload = true,
    monitorProcessUnload = true,
    monitorThreads = false,
    modules = {}
}

pluginsConfig.ModuleExecutionDetector = {
    trackAllModules = true,
    configureAllModules = true
}

pluginsConfig.PESelector = {
    moduleNames = {
        'ab.exe',
        'ab2.exe',
        'aorb.exe',
        'ab-fish-black.exe',
        'ab-fish-red.exe',
        'ab-fish-white.exe',
        'ab-tiger-black.exe',
        'ab-tiger-red.exe',
        'ab-tiger-white.exe',
        'ab2-fish-white.exe',
        'aorb-fish-white.exe',
        'ab-vmp.exe',
        'ab2-vmp.exe',
        'fib.exe',
        'pwcheck.exe',
        'sha2.exe',
    }
}

pluginsConfig.SymbolizeArgsPE = {
    symbolizeArgv0 = false
}

pluginsConfig.ExportPE = {
    baseDirs = {'test'}
}

pluginsConfig.AddrLog = {
    logAll = false,
    logFile = 'addrlog.dat'
}

pluginsConfig.RegisterLog = {
    logFile = 'register-log.txt'
}

pluginsConfig.TranslationBlockTracer = {
    manualTrigger = false,
    flushTbCache = false,
}
