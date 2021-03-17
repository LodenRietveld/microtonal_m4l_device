inlets = 1;
outlets = 2;

function list(){
	var num = arguments[0];
	var denom = arguments[1];
	
	if (num > 0){
		var j = num;
		var lowestNum = getLowestInteger(num);
		var lowestDenom = getLowestInteger(denom);
		
		if (lowestNum == lowestDenom){
			outlet(1, 1);
			outlet(0, 1);
			return;
		}
		
		if (lowestNum < lowestDenom){
			while (lowestNum < lowestDenom){
				lowestNum *= 2;
			}
		}
		
		while (lowestNum > lowestDenom){
			lowestDenom *= 2;
		}
		
		lowestDenom /= 2;
		
		outlet(1, lowestDenom);
		outlet(0, lowestNum);
	}
}


function isInt(n) {
   return n % 1 === 0;
}

function getLowestInteger(n){
	var j = n;
	var c = 1;
	while(isInt(j)){
		j = j/2;
		
		if (isInt(j)){
			c *= 2;
		}
	}
	
	return n/c;
}