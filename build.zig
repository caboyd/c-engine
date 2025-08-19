const std = @import("std");
const ArrayList = std.ArrayListUnmanaged;
const mem = std.mem;
const Allocator = mem.Allocator;
const FixedBufferAllocator = std.heap.FixedBufferAllocator;
const fs = std.fs;
const Module = std.Build.Module;

const zcc = @import("compile_commands");

//TODO:: add release only flags and specific release build
const release_flag: []const []const u8 = &.{"-DTODO"};

const compile_flags: []const []const u8 = &.{"-std=c99","-DCENGINE_SLOW=1", "-DCENGINE_INTERNAL=1"};
const unoptimized_flags: []const []const u8 = &.{"-DDEBUG=1"};
const runtime_and_warn_flags = runtime_check_flags ++ warning_flags;

//copied from https://github.com/JacobHumphreys/cpp-build-template.zig
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    std.log.info("optimize={}\n", .{optimize});


    const dll = b.addLibrary(.{ 
        .name = "cengine", 
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
        .linkage = .dynamic,
    });


    const exe = b.addExecutable(.{ 
        .name = "win32-cengine", 
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),

    });
    exe.subsystem = .Windows;

    const dev = b.addExecutable(.{ 
        .name = "win32-cengine-dev", 
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),

    });
    dev.subsystem = .Windows;

    const exe_flags = getBuildFlags(
        b.allocator,
        exe,
        optimize,
        false
    ) catch |err|
        @panic(@errorName(err));

    const dev_flags = getBuildFlags(
        b.allocator,
        dev,
        optimize,
        true
    ) catch |err|
        @panic(@errorName(err));



    const unity_files = Module.AddCSourceFilesOptions {
        .files = &[_][]const u8{"src/win32_cengine.c"},
        .flags = exe_flags,
        .language = .c,
    };

    const dll_files = Module.AddCSourceFilesOptions {
        .files = &[_][]const u8{"src/cengine.c"},
        .flags = dev_flags,
        .language = .c,
    };

    const link_libs: []const []const u8 = &.{
        "gdi32",
        "ole32",
        "avrt",
    };

    for(link_libs) |lib| {
        exe.linkSystemLibrary(lib);
        dev.linkSystemLibrary(lib);
    }
    const files = unity_files;
    exe.addCSourceFiles(files);

    var dev_files = files;
    //we dont care about warning when developing
    dev_files.flags = dev_flags;
    dev.addCSourceFiles(dev_files);

    dll.addCSourceFiles(dll_files);


    const dll_install_step = &b.addInstallArtifact(dll, .{}).step;
    const exe_install_step = &b.addInstallArtifact(exe, .{}).step;
    const dev_install_step = &b.addInstallArtifact(dev, .{}).step;

    const exe_run = b.addRunArtifact(exe);
    const dev_run = b.addRunArtifact(dev);

    dev_run.step.dependOn(dll_install_step);
    dev_run.step.dependOn(dev_install_step);

    exe_run.step.dependOn(dll_install_step);
    exe_run.step.dependOn(exe_install_step);

    if (b.args) |args| {
        exe_run.addArgs(args);
        dev_run.addArgs(args);
    }

    const build_step = b.step("build", "build all");
    build_step.dependOn(dll_install_step);
    build_step.dependOn(dev_install_step);
    build_step.dependOn(exe_install_step);

    const dll_step = b.step("dll", "build the dll");
    dll_step.dependOn(dll_install_step);

    const run_step = b.step("run", "runs the application");
    run_step.dependOn(&exe_run.step);

    const dev_step = b.step("dev", "runs the application without any warning or san flags");
    dev_step.dependOn(&dev_run.step);

    var targets = ArrayList(*std.Build.Step.Compile).empty;
    defer targets.deinit(b.allocator);

    targets.append(b.allocator, exe) catch |err| @panic(@errorName(err));
   // targets.append(b.allocator, dev) catch |err| @panic(@errorName(err));

    _ = zcc.createStep(
        b,
        "cmds",
        targets.toOwnedSlice(b.allocator) catch |err|
            @panic(@errorName(err)),
    );
}

fn getClangPath(alloc: std.mem.Allocator, target: std.Target) ![]const u8 {
    const asan_lib = if (target.os.tag == .windows)
        "clang_rt.asan_dynamic-x86_64.dll"
    else
        "libclang_rt.asan-x86_64.so";
    var child_proc = std.process.Child.init(&.{
        "clang",
        try std.mem.concat(alloc, u8, &.{ "-print-file-name=", asan_lib }),
    }, alloc);
    child_proc.stdout_behavior = .Pipe;

    try child_proc.spawn();

    const child_std_out = child_proc.stdout.?;

    var output = try child_std_out.readToEndAlloc(alloc, 512);

    _ = try child_proc.wait();

    const file_delim = if (target.os.tag == .windows) "\\" else "/";

    if (mem.lastIndexOf(u8, output, file_delim)) |last_path_sep| {
        output.len = last_path_sep + 1;
    } else {
        @panic("Path Not Formatted Correctly");
    }
    return output;
}

const runtime_check_flags: []const []const u8 = &.{
    "-fsanitize=array-bounds,null,alignment,unreachable", // address and leak are linux/macos only in 0.14.1
    "-fstack-protector-strong",
    "-fno-omit-frame-pointer",
};

const warning_flags: []const []const u8 = &.{
    "-Wall",
    "-Wextra",
    "-Wnull-dereference",
    "-Wuninitialized",
    "-Wshadow",
    "-Wpointer-arith",
    "-Wstrict-aliasing",
    "-Wstrict-overflow=5",
    "-Wcast-align",
    "-Wconversion",
    "-Wsign-conversion",
    "-Wfloat-equal",
    "-Wformat=2",
    "-Wswitch-enum",
    "-Wmissing-declarations",
    "-Wunused",
    "-Wundef",
    "-Werror",
    "-Wno-unused-function",
    "-Wno-pragma-pack",
    "-Wno-unused-parameter",
};


fn getBuildFlags(
    alloc: Allocator,
    exe: *std.Build.Step.Compile,
    optimize: std.builtin.OptimizeMode,
    dev_mode: bool
) ![]const []const u8 {
    var c_flags: []const []const u8 = undefined;

    if (optimize == .Debug) {
        c_flags = compile_flags ++ unoptimized_flags ++ runtime_and_warn_flags;
        if(dev_mode){
            return compile_flags ++ unoptimized_flags;
        }

        if (exe.rootModuleTarget().os.tag == .windows) return c_flags;

        exe.addLibraryPath(.{ .cwd_relative = try getClangPath(alloc, exe.rootModuleTarget()) });
        const asan_lib = if (exe.rootModuleTarget().os.tag == .windows)
            "clang_rt.asan_dynamic-x86_64" // Won't be triggered in current version
        else
            "clang_rt.asan-x86_64";

        exe.linkSystemLibrary(asan_lib);
        //exe.linkSystemLibrary("clang_rt.ubsan_standalone_cxx-x86_64");
    } else {
        c_flags = compile_flags;
    }
    
    return c_flags;
}
