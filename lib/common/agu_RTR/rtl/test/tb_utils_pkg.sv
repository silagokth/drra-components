
package tb_utils_pkg;

  class TestReporter;
    int test_num = 0;
    int error_count = 0;
    string tb_name;

    function new(string name);
      tb_name = name;
    endfunction

    function void print_banner();
      $display("");
      $display("+========================================+");
      $display("|   %s", tb_name);
      $display("+========================================+");
    endfunction

    function void print_test_header(string test_name, string expression, int expected_addrs);
      test_num++;
      $display("");
      $display("========================================");
      $display("TEST %0d: %s", test_num, test_name);
      $display("========================================");
      $display("  Timing expression: %s", expression);
      $display("  Expected total addresses: %0d", expected_addrs);
      $display("----------------------------------------");
    endfunction

    function void print_summary();
      $display("");
      $display("+========================================+");
      $display("|   TEST SUMMARY                         |");
      $display("+========================================+");
      $display("|   Total Tests: %3d                     |", test_num);
      if (error_count == 0) $display("|   Status: ALL PASSED                   |");
      else $display("|   Status: %3d ERRORS                   |", error_count);
      $display("+========================================+");
    endfunction

    function void log_error(string msg);
      $display("  [ERROR] %s", msg);
      error_count++;
    endfunction

    function void log_pass(string msg);
      $display("  [PASS] %s", msg);
    endfunction
  endclass

endpackage
