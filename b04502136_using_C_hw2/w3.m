len=100;
l1={};
l2={};
l3={};
Dynamic[
	ListPlot[
		{l1, l2, l3},
		AxesLabel -> {"time[s]", "value"},
		ImageSize -> Large,
		PlotLegends -> {"AccX", "AccY", "AccZ"},
		PlotRange -> {{(-len + 1)/10, 0}, {-1000, 1000}}
	]
]
While[
	True,
	data=ReadList[
		"log.dat",{Number,Number,Number}
	];
	l1=Transpose[
		{
			Table[i,{i,(-len+1)/10,0,1/10}],
			Take[
				Transpose[data][[1]],-len
			]
		}
	];
	l2=Transpose[
		{
			Table[i,{i,(-len+1)/10,0,1/10}],
			Take[
				Transpose[data][[2]],-len
			]
		}
	];
	l3=Transpose[
		{
			Table[i,{i,(-len+1)/10,0,1/10}],
			Take[
				Transpose[data][[3]],-len
			]
		}
	];
	Pause[0.1];
]


