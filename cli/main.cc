/*
 * Copyright (c) 2015 Masayoshi Mizutani <mizutani@sfc.wide.ad.jp>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/msgcls.hpp"
#include "./optparse.h"

int main(int argc, char *argv[]) {
  // Configure options
  optparse::OptionParser psr = optparse::OptionParser();
  psr.add_option("-F").dest("fluentd_format").action("store_true")
    .help("Enable parser with fluentd format");
  
  optparse::Values& opt = psr.parse_args(argc, argv);
  std::vector <std::string> args = psr.args();

  if (args.size() != 1) {
    std::cerr << "syntax) msgcls <key> <target_file> ..." << std::endl;
    return EXIT_FAILURE;
  }
  
  msgcls::MsgCls *mc = new msgcls::MsgCls(args[0]);

  if (opt.get("fluentd_format")) {
    mc->set_format(msgcls::Format::FLUENTD);
  }
  
  for (size_t i = 1; i < args.size(); i++) {
    int fd = ::open(args[i].c_str(), O_RDONLY);
    mc->run(fd);
    ::close(fd);
  }
  return EXIT_SUCCESS;
}

