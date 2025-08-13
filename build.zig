const std = @import("std");
const ArrayList = std.ArrayListUnmanaged;
const mem = std.mem;
const Allocator = mem.Allocator;
const FixedBufferAllocator = std.heap.FixedBufferAllocator;
const fs = std.fs;
const Module = std.Build.Module;

const zcc = @import("compile_commands");

const compile_flags: []const []const u8 = &.{"-std=c99"};
const debug_flags = runtime_check_flags ++ warning_flags;

//copied from https://github.com/JacobHumphreys/cpp-build-template.zig
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    std.log.info("optimize={}\n", .{optimize});

    const exe = b.addExecutable(.{ .name = "c-engine", .root_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    }) });

    const dev = b.addExecutable(.{ .name = "c-engine-dev", .root_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    }) });

    const exe_flags = getBuildFlags(
        b.allocator,
        exe,
        optimize,
    ) catch |err|
        @panic(@errorName(err));

    const exe_files = getSrcFiles(b.allocator, .{
        .dir_path = "src",
        .flags = exe_flags,
        .language = .c,
    }) catch |err|
        @panic(@errorName(err));

    const link_libs: []const []const u8 = &.{
        "gdi32",
        "ole32",
        "avrt",
    };

    for(link_libs) |lib| {
        exe.linkSystemLibrary(lib);
        dev.linkSystemLibrary(lib);
    }

    exe.addCSourceFiles(exe_files);

    var dev_files = exe_files;
    //we dont care about warning when developing
    dev_files.flags = compile_flags;
    dev.addCSourceFiles(dev_files);

    // exe.addIncludePath(b.path("src/base"));

    //Build and Link zig -> c code --------------------------------
    //const lib = b.addSharedLibrary(.{
    //    .name = "example",
    //    .root_source_file = b.path("src/zig/example.zig"),
    //    .target = target,
    //    .optimize = optimize,
    //    .link_libc = true,
    //});
    //---------------------------------------------

    b.installArtifact(exe);
    const exe_run = b.addRunArtifact(exe);
    const dev_run = b.addRunArtifact(dev);

    exe_run.step.dependOn(b.getInstallStep());
    dev_run.step.dependOn(&b.addInstallArtifact(dev, .{}).step);

    if (b.args) |args| {
        exe_run.addArgs(args);
        dev_run.addArgs(args);
    }

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
};

pub fn getSrcFiles(alloc: std.mem.Allocator, opts: struct { dir_path: []const u8 = "./src/", language: Module.CSourceLanguage, flags: []const []const u8 = &.{} }) !Module.AddCSourceFilesOptions {
    const src = try fs.cwd().openDir(opts.dir_path, .{ .iterate = true });

    var file_list = ArrayList([]const u8).empty;
    errdefer file_list.deinit(alloc);

    const extension = @tagName(opts.language); //Will break for obj-c and assembly

    var src_iterator = src.iterate();
    while (try src_iterator.next()) |entry| {
        switch (entry.kind) {
            .file => {
                if (!mem.endsWith(u8, entry.name, extension))
                    continue;

                const path = try fs.path.join(alloc, &.{ opts.dir_path, entry.name });
                try file_list.append(alloc, path);
            },
            .directory => {
                var dir_opts = opts;
                dir_opts.dir_path = try fs.path.join(alloc, &.{ opts.dir_path, entry.name });

                try file_list.appendSlice(alloc, (try getSrcFiles(alloc, dir_opts)).files);
            },
            else => continue,
        }
    }

    return Module.AddCSourceFilesOptions{
        .files = try file_list.toOwnedSlice(alloc),
        .language = opts.language,
        .flags = opts.flags,
    };
}

fn getBuildFlags(
    alloc: Allocator,
    exe: *std.Build.Step.Compile,
    optimize: std.builtin.OptimizeMode,
) ![]const []const u8 {
    var c_flags: []const []const u8 = undefined;

    if (optimize == .Debug) {
        c_flags = compile_flags ++ debug_flags;
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
