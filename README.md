# ACM-POJ-1009
图像边缘检测

-- 2016/6/23 --
这题的大意是对超大的图片(10^9这么多像素)进行边缘检测，边缘检测的方法是输出每个像素和其周围8个像素的最大绝对值差。
最初的思路是按行计算，即如果某一行里有像素灰度值跳变，就把它和它前后共三行暴力计算，显然超时了。。。
重新理了一下思路后，发现题目里说灰度值跳变的像素不超过1000个（虽然实际上不太现实），感觉可以只在像素跳变的点及其周围的点计算。
准备明天论证一下再说。

-- 2016/6/24 --
今天论证编码弄了一整天，终于AC了。思路有空再写吧

-- 备注 --
第一次超时的版本在TLE文件夹下，第二次AC的版本在AC文件夹下。
