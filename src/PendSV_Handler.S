    /*
        .syntax unified

        Two slightly different syntaxes are support for ARM and THUMB instructions.
        The default, divided, uses the old style where ARM and THUMB instructions had their own,
        separate syntaxes. The new, unified syntax, which can be selected via the
        .syntax directive
    */

    .syntax unified
    .global PendSV_Handler



    /*
        Se cambia a la seccion .text, donde se almacena el programa en flash
    */
    .text

    /*
        Indicamos que la proxima funcion debe ser tratada como codigo thumb al ser compilada
        Ver documentacion al respecto para mas detalles
    */
    .thumb_func



PendSV_Handler:

    /*
    * Cuando se ingrea al handler de PendSV lo primero que se ejecuta es un push para
    * guardar los registros R4-R11 y el valor de LR, que en este punto es EXEC_RETURN
    * El push se hace al reves de como se escribe en la instruccion, por lo que LR
    * se guarda en la posicion 9 (luego del stack frame). Como la funcion get_next_context
    * se llama con un branch con link, el valor del LR es modificado guardando la direccion
    * de retorno una vez se complete la ejecucion de la funcion

    * El pasaje de argumentos a get_next_context se hace como especifica el AAPCS siendo
    * el unico argumento pasado por RO, y el valor de retorno tambien se almacena en R0
    *
    * NOTA: El primer ingreso a este handler (luego del reset) implica que el push se hace sobre el
    * stack inicial, ese stack se pierde porque no hay seguimiento del MSP en el primer ingreso
    */


    /*
    * Las tres primeras corresponden a un testeo del bit EXEC_RETURN[4]. La instruccion TST hace un
    * AND estilo bitwise (bit a bit) entre el registro LR y el literal inmediato. El resultado de esta
    * operacion no se guarda y los bits N y Z son actualizados. En este caso, si el bit EXEC_RETURN[4] = 0
    * el resultado de la operacion sera cero, y la bandera Z = 1, por lo que se da la condicion EQ y
    * se hace el push de los registros de FPU restantes
    */

    tst lr,0x10
    it eq
    vpusheq {s16-s31}

    push {r4-r11,lr}
    mrs r0,msp
    bl get_next_context
    msr msp,r0
    pop {r4-r11,lr}         //Recuperados todos los valores de registros


    /*
    * Habiendo hecho el cambio de contexto y recuperado los valores de los registros, es necesario
    * determinar si el contexto tiene guardados registros correspondientes a la FPU. si este es el caso
    * se hace el unstacking de los que se hizo PUSH manualmente.
    */

    tst lr,0x10
    it eq
    vpopeq {s16-s31}

    bx lr                   //se hace un branch indirect con el valor de LR que es nuevamente EXEC_RETURN
