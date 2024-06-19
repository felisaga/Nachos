TLB-SIZE    (Ratio Sort)    (Ratio Matmult) 
4           0.921820        0.917318
8           0.999098        0.977852
16          0.999493        0.988377
32          0.999845        0.999831
64          0.999999        0.999918

Los test fueron ejecutados con el argumento -m 64

Desde ya podemos descartar como candidatos a tamaño de TLB a 16,32,64 no solo que son muy grandes para solo 64 marcos (representarian 1/4, 1/2 y 1 de la memoria fisica respectivamente, que para un buffer 'caro' es una exageración). Si no que tambien las ganancias con respecto a un tamaño de 8 no son grandes.

Nos queda 4 y 8, 8 puede seguir siendo grande relativamente a la memoria fisica pero eso dependera el costo de la implementacion. En general recomendaria 4 o un valor intermedio entre 4 y 8, siempre teniendo en cuenta la factibilidad de implementar el buffer del tamaño elegido.

