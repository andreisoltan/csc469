#/bin/bash -x

for lab in b2200 b2210 b2220 b2240 b3175 b3185 b3195 b3200 s2360; do
    ssh "$lab-02" platform-info.sh > "$lab.platform"
done
