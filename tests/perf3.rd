(load-file "../tests/core.rd")
(load-file "../tests/perf.rd")

;;(prn "Start: basic macros/atom test")

(def! atm (atom (list 0 1 2 3 4 5 6 7 8 9)))

(println "iters/s:"
  (run-fn-for
    (fn* []
      (do
        (or false nil false nil false nil false nil false nil (first @atm))
        (cond false 1 nil 2 false 3 nil 4 false 5 nil 6 "else" (first @atm))
        (-> (deref atm) rest rest rest rest rest rest first)
        (swap! atm (fn* [a] (concat (rest a) (list (first a)))))))
    10))

;;(prn "Done: basic macros/atom test")
