(load-file "../tests/core.rd")
(load-file "../test/perf.rd")

;;(prn "Start: basic macros performance test")

(time (do
  (or false nil false nil false nil false nil false nil 4)
  (cond false 1 nil 2 false 3 nil 4 false 5 nil 6 "else" 7)
  (-> (list 1 2 3 4 5 6 7 8 9) rest rest rest rest rest rest first)))

;;(prn "Done: basic macros performance test")
