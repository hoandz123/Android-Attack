package com.android.attack;

import android.content.Context;

import java.io.IOException;
import java.lang.reflect.Array;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public final class DexLoader {

    private DexLoader() {
    }

    public static void loadDex(Context context, ByteBuffer[] buffers) throws NoSuchFieldException, IllegalAccessException, IllegalArgumentException, NoSuchMethodException, InvocationTargetException {
        Field pathListField = findField(context.getClassLoader(), "pathList");
        Object pathList = pathListField.get(context.getClassLoader());
        Field dexElementsField = findField(pathList, "dexElements");
        Object[] dexElements = (Object[]) dexElementsField.get(pathList);
        ArrayList<IOException> suppressedExceptions = new ArrayList<>();
        Method makeDexElements = findMethod(pathList, "makeInMemoryDexElements", ByteBuffer[].class, List.class);
        Object[] addElements = (Object[]) makeDexElements.invoke(pathList, buffers, suppressedExceptions);
        Object[] newElements = (Object[]) Array.newInstance(dexElements.getClass().getComponentType(), dexElements.length + addElements.length);
        System.arraycopy(dexElements, 0, newElements, 0, dexElements.length);
        System.arraycopy(addElements, 0, newElements, dexElements.length, addElements.length);
        dexElementsField.set(pathList, newElements);
    }

    private static Field findField(Object target, String name) throws NoSuchFieldException {
        Class<?> clazz = target instanceof Class ? (Class<?>) target : target.getClass();
        while (clazz != null) {
            try {
                Field field = clazz.getDeclaredField(name);
                field.setAccessible(true);
                return field;
            } catch (NoSuchFieldException ignored) {
                clazz = clazz.getSuperclass();
            }
        }
        throw new NoSuchFieldException("Field " + name + " not found in " + target.getClass());
    }

    private static Method findMethod(Object target, String name, Class<?>... parameterTypes) throws NoSuchMethodException {
        Class<?> clazz = target instanceof Class ? (Class<?>) target : target.getClass();
        while (clazz != null) {
            try {
                Method method = clazz.getDeclaredMethod(name, parameterTypes);
                method.setAccessible(true);
                return method;
            } catch (NoSuchMethodException ignored) {
                clazz = clazz.getSuperclass();
            }
        }
        throw new NoSuchMethodException("Method " + name + " not found in " + target.getClass());
    }
}
