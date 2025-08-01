const std = @import("std");


pub fn build(b: *std.Build) void {
  // Define the executable target
  const exe = b.addExecutable(.{
    .name = "c-engine",
    .target = b.resolveTargetQuery(.{}), // Default target (current system)
    .optimize = .Debug,
  });
  // Add your C source files
  exe.addCSourceFiles(.{
    .files = &[_][]const u8{
      "src/main.c",
    },
    .flags = &[_][]const u8{
      "-Wall",
      "-Wextra"
    },
  });

  // Link against the C standard Library
  exe.linkLibC();

  // Install the executable
  b.installArtifact(exe);

  // Add a "run" step for convenience
  const run_cmd = b.addRunArtifact(exe);
  run_cmd.step.dependOn(b.getInstallStep());
  if (b.args) |args| {
      run_cmd.addArgs(args);
  }
  const run_step = b.step("run", "Run the application");
  run_step.dependOn(&run_cmd.step);
}
