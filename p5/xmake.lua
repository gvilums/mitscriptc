add_rules("mode.debug", "mode.release")

set_languages("c++20")

-- rule("antlr_generate")
--     set_extensions(".g4", ".g")
--     on_buildcmd_file(function (target, cmds, sourcefile_grammar, opt)
--         import("lib.detect.find_tool")
--         local java = assert(find_tool("java"), "java not found!")
        
--         -- depend.on_changed(function()
--         --     os.vrunv("java", {"-jar", "external/antlr.jar", "-package", "lexer", "-Dlanguage=Cpp", sourcefile})
--         -- end, {files = sourcefile})
--         local sourcefile_cx = path.join(target:autogendir(), "grammar", path.basename(sourcefile_grammar) .. ".cpp")
--         local headerfile = path.join(target:autogendir(), "grammar", path.basename(sourcefile_grammar) .. ".h")

--         -- add objectfile
--         local objectfile = target:objectfile(sourcefile_cx)
--         table.insert(target:objectfiles(), objectfile)

--         -- add commands
--         cmds:show_progress(opt.progress, "${color.build.object}compiling.antlr_generate %s", sourcefile_grammar)
--         cmds:mkdir(path.directory(sourcefile_cx))
--         cmds:vrunv(java.program, {"-jar", "external/antlr.jar", "-package", "lexer", "-Dlanguage=Cpp", "-o", target:autogendir(), sourcefile_grammar} )
--         cmds:compile(sourcefile_cx, objectfile)
--         cmds:cp(headerfile, path.join(path.directory(sourcefile_grammar), path.basename(sourcefile_grammar) .. ".h"))
--         print(path.join(path.directory(sourcefile_grammar), path.basename(sourcefile_grammar) .. ".h"))

--         -- add deps
--         cmds:add_depfiles(sourcefile_grammar)
--         cmds:set_depmtime(os.mtime(objectfile))
--         cmds:set_depcache(target:dependfile(objectfile))
--     end)

target("antlr_rt")
    set_kind("static")
    add_files("external/antlr_rt/**.cpp")
    add_includedirs("external/antlr_rt")

target("asmjit")
    set_kind("static")
    add_rules("c++.unity_build")
    add_files("external/asmjit/**.cpp")
    
target("mitscriptc")
    set_kind("binary")
    -- add_rules("c++.unity_build")
    add_files("grammar/MITScript.cpp")
    add_includedirs("grammar")

    add_files("src/*.cpp")
    add_deps("asmjit")
    add_deps("antlr_rt")
    add_includedirs("external/antlr_rt")
    add_includedirs("external/asmjit")

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

